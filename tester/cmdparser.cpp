/**
 * @file
 * @copyright  Copyright 2016 GNSS Sensor Ltd. All right reserved.
 * @author     Sergey Khabarov - sergeykhbr@gmail.com
 * @brief      Simplified input command string parser.
 */

#include <string.h>
#include "cmdparser.h"
#include "ithread.h"

namespace debugger {

#define KB_UP       0x48
#define KB_DOWN     80
#define KB_LEFT     75
#define KB_RIGHT    77
#define KB_ESCAPE   27
static const uint8_t ARROW_PREFIX = 0xe0;
static const uint8_t UNICODE_BACKSPACE = 0x7f;

/** Class registration in the Core */
REGISTER_CLASS(CmdParserService)

union DsuRunControlRegType {
    struct bits_type {
        uint64_t halt     : 1;
        uint64_t stepping : 1;
        uint64_t rsv1     : 2;
        uint64_t core_id  : 16;
        uint64_t rsv2     : 44;
    } bits;
    uint64_t val;
    uint8_t  buf[8];
};

enum RegListType {REG_Name, REG_IDx};

CmdParserService::CmdParserService(const char *name) : IService(name) {
    registerInterface(static_cast<IKeyListener *>(this));
    registerAttribute("Console", &console_);
    registerAttribute("Tap", &tap_);
    registerAttribute("Loader", &loader_);
    registerAttribute("History", &history_);
    registerAttribute("HistorySize", &history_size_);
    registerAttribute("ListCSR", &listCSR_);
    registerAttribute("RegNames", &regNames_);

    console_.make_string("");
    tap_.make_string("");
    loader_.make_string("");
    history_.make_list(0);
    history_size_.make_int64(4);
    listCSR_.make_list(0);
    regNames_.make_list(0);
    history_idx_ = 0;
    symb_seq_msk_ = 0xFF;
    tmpbuf_ = new uint8_t[tmpbuf_size_ = 4096];
    outbuf_ = new char[outbuf_size_ = 4096];
}

CmdParserService::~CmdParserService() {
    delete [] tmpbuf_;
    delete [] outbuf_;
}

void CmdParserService::postinitService() {
    iconsole_ = static_cast<IConsole *>
            (RISCV_get_service_iface(console_.to_string(), IFACE_CONSOLE));
    itap_ = static_cast<ITap *>
            (RISCV_get_service_iface(tap_.to_string(), IFACE_TAP));
    iloader_ = static_cast<IElfLoader *>
            (RISCV_get_service_iface(loader_.to_string(), IFACE_ELFLOADER));
    history_idx_ = history_.size();
}

int CmdParserService::keyUp(int value) {
    if (!iconsole_) {
        return 0;
    }
    uint8_t symb = static_cast<uint8_t>(value);
    bool set_history_end = true;

    //printf("!!!! key = %02x\n", value);

    if (!convertToWinKey(symb)) {
        return 0;
    }
    switch (symb_seq_) {
    case 0:
        break;
    case (ARROW_PREFIX << 8) | KB_UP:
        set_history_end = false;
        if (history_idx_ == history_.size()) {
            unfinshedLine_ = cmdLine_;
        }
        if (history_idx_ > 0) {
            history_idx_--;
        }
        cmdLine_ = std::string(history_[history_idx_].to_string());
        break;
    case (ARROW_PREFIX << 8) | KB_DOWN:
        set_history_end = false;
        if (history_idx_ == (history_.size() - 1)) {
            history_idx_++;
            cmdLine_ = unfinshedLine_;
        } else if (history_idx_ < (history_.size() - 1)) {
            history_idx_++;
            cmdLine_ = std::string(history_[history_idx_].to_string());
        }
        break;
    case '\b':// 1. Backspace button:
        if (cmdLine_.size()) {
            cmdLine_.erase(cmdLine_.size() - 1);
        }
        break;
    case '\n':
    case '\r':// 2. Enter button:
        memcpy(cmdbuf_, cmdLine_.c_str(), cmdLine_.size() + 1);
        cmdLine_.clear();
        iconsole_->writeCommand(cmdbuf_);
        iconsole_->setCmdString(cmdLine_.c_str());
        processLine(cmdbuf_);
        iconsole_->writeBuffer(outbuf_);
        break;
    default:;
        cmdLine_ += symb;
    }

    if (set_history_end) {
        history_idx_ = history_.size();
    }
    iconsole_->setCmdString(cmdLine_.c_str());
    return 0;
}

bool CmdParserService::convertToWinKey(uint8_t symb) {
    bool ret = true;
    symb_seq_ <<= 8;
    symb_seq_ |= symb;
    symb_seq_ &= symb_seq_msk_;

#if defined(_WIN32) || defined(__CYGWIN__)
    if (symb_seq_ == ARROW_PREFIX) {
        ret = false;
        symb_seq_msk_ = 0xFFFF;
    } else {
        symb_seq_msk_ = 0xFF;
    }
#else
    if (symb_seq_ == UNICODE_BACKSPACE) {
        symb_seq_ = '\b';
        symb_seq_msk_ = 0xFF;
    } else if (symb_seq_ == 0x1b) {
        symb_seq_msk_ = 0xFFFF;
        ret = false;
    } else if (symb_seq_ == 0x1b5b) {
        symb_seq_msk_ = 0xFFFFFF;
        ret = false;
    } else if (symb_seq_ == 0x1b5b41) {
        symb_seq_ = (ARROW_PREFIX << 8) | KB_UP;
        symb_seq_msk_ = 0xFF;
    } else if (symb_seq_ == 0x1b5b42) {
        symb_seq_ = (ARROW_PREFIX << 8) | KB_DOWN;
        symb_seq_msk_ = 0xFF;
    } else if (symb_seq_ == 0x1b5b43) {
        //symb_seq_ = (ARROW_PREFIX << 8) | KB_RIGHT;
        //symb_seq_msk_ = 0xFF;
        ret = false;
    } else if (symb_seq_ == 0x1b5b44) {
        //symb_seq_ = (ARROW_PREFIX << 8) | KB_LEFT;
        //symb_seq_msk_ = 0xFF;
        ret = false;
    } else {
        symb_seq_msk_ = 0xFF;
    }
#endif
    if (symb_seq_ == ((uint32_t('\r') << 8) | '\n')) {
        symb_seq_ = 0;
    }
    return ret;
}

void CmdParserService::processLine(const char *line) {
    outbuf_[outbuf_cnt_ = 0] = '\0';
    if (line[0] == 0) {
        return;
    }
    addToHistory(line);

    AttributeType listArgs(Attr_List);
    splitLine(cmdbuf_, &listArgs);
    if (!listArgs[0u].is_string()) {
        outf("Wrong command format\n");
        return;
    }

    if (strcmp(listArgs[0u].to_string(), "help") == 0) {
        outf("** List of supported commands: **\n");
        outf("      loadelf   - Load ELF-file\n");
        outf("      log       - Enable log-file\n");
        outf("      memdump   - Dump memory to file\n");
        outf("      csr       - Access to CSR registers\n");
        outf("      write     - Write memory\n");
        outf("      read      - Read memory\n");
        outf("** List of supported commands by simulator only: **\n");
        outf("      stop/break/s    - Stop simulation\n");
        outf("      run/go/c  - Run simulation for a specify "
                               "number of steps\n");
        outf("      regs      - List of registers values\n");
        outf("      br        - Breakpoint operation\n");
        outf("\n");
    } else if (strcmp(listArgs[0u].to_string(), "loadelf") == 0) {
        if (listArgs.size() == 2) {
            loadElf(&listArgs);
        } else {
            outf("Description:\n");
            outf("    Load ELF-file to SOC target memory.\n");
            outf("Example:\n");
            outf("    load /home/riscv/image.elf\n");
        }
    } else if (strcmp(listArgs[0u].to_string(), "log") == 0) {
        if (listArgs.size() == 2) {
            iconsole_->enableLogFile(listArgs[1].to_string());
        } else {
            outf("Description:\n");
            outf("    Write console output into specified file.\n");
            outf("Example:\n");
            outf("    log session.log\n");
            outf("    log /home/riscv/session.log\n");
        }
    } else if (strcmp(listArgs[0u].to_string(), "memdump") == 0) {
        if (listArgs.size() == 4 || listArgs.size() == 5) {
            memDump(&listArgs);
        } else {
            outf("Description:\n");
            outf("    Dump memory to file (default in Binary format).\n");
            outf("Usage:\n");
            outf("    memdump <addr> <bytes> [filepath] [bin|hex]\n");
            outf("Example:\n");
            outf("    memdump 0x0 8192 dump.bin\n");
            outf("    memdump 0x40000000 524288 dump.hex hex\n");
            outf("    memdump 0x10000000 128 \"c:/My Documents/dump.bin\"\n");
        }
    } else if (strcmp(listArgs[0u].to_string(), "regs") == 0) {
        if (listArgs.size() == 1) {
            regs(&listArgs);
        } else {
            outf("Description:\n");
            outf("    Print values of all CPU's registers.\n");
            outf("Usage:\n");
            outf("    regs\n");
            outf("    regs <cpuid>\n");
            outf("Example:\n");
            outf("    regs\n");
        }
    } else if (strcmp(listArgs[0u].to_string(), "csr") == 0) {
        if (listArgs.size() == 2) {
            readCSR(&listArgs);
        } else if (listArgs.size() == 3) {
            writeCSR(&listArgs);
        } else {
            outf("Description:\n");
            outf("    Access to CSRs registers of the CPU.\n");
            outf("Usage:\n");
            outf("    READ:  csr <addr|name>\n");
            outf("    WRITE: csr <addr|name> <value>\n");
            outf("Example:\n");
            outf("    csr MCPUID\n");
            outf("    csr 0x762 1\n");
        }
    } else if (strcmp(listArgs[0u].to_string(), "write") == 0) {
        if (listArgs.size() == 4) {
            writeMem(&listArgs);
        } else {
            outf("Description:\n");
            outf("    Write memory.\n");
            outf("Usage:\n");
            outf("    write <addr> <bytes> [value]\n");
            outf("Example:\n");
            outf("    write 0xfffff004 4 0x20160323\n");
            outf("    write 0x10040000 16 "
                        "[0xaabbccdd00112233, 0xaabbccdd00112233]\n");
        }
    } else if (strcmp(listArgs[0u].to_string(), "read") == 0) {
        if (listArgs.size() == 2) {
            AttributeType item(Attr_Integer, 4);
            listArgs.add_to_list(&item);
            readMem(&listArgs);
        } else if (listArgs.size() == 3) {
            readMem(&listArgs);
        } else {
            outf("Description:\n");
            outf("    32-bits aligned memory reading. "
                        "Default bytes = 4 bytes.\n");
            outf("Usage:\n");
            outf("    read <addr> <bytes>\n");
            outf("Example:\n");
            outf("    read 0xfffff004 16\n");
            outf("    read 0xfffff004\n");
        }
    } else if (strcmp(listArgs[0u].to_string(), "exit") == 0) {
        RISCV_break_simulation();
    } else if (strcmp(listArgs[0u].to_string(), "halt") == 0
            || strcmp(listArgs[0u].to_string(), "stop") == 0
            || strcmp(listArgs[0u].to_string(), "s") == 0) {
        if (listArgs.size() == 1) {
            halt(&listArgs);
        } else {
            outf("Description:\n");
            outf("    Stop simulation.\n");
            outf("Example:\n");
            outf("    halt\n");
            outf("    stop\n");
            outf("    s\n");
        }
    } else if (strcmp(listArgs[0u].to_string(), "run") == 0
            || strcmp(listArgs[0u].to_string(), "go") == 0
            || strcmp(listArgs[0u].to_string(), "c") == 0) {
        if (listArgs.size() == 1 
            || (listArgs.size() == 2 && listArgs[1].is_integer())) {
            run(&listArgs);
        } else {
            outf("Description:\n");
            outf("    Run simulation for a specified number of steps.\n");
            outf("Usage:\n");
            outf("    run <N steps>\n");
            outf("Example:\n");
            outf("    run\n");
            outf("    go 1000\n");
            outf("    c 1\n");
        }
    } else if (strcmp(listArgs[0u].to_string(), "br") == 0) {
        if (listArgs.size() == 3 && listArgs[1].is_string()) {
            br(&listArgs);
        } else {
            outf("Description:\n");
            outf("    Add or remove memory breakpoint.\n");
            outf("Usage:\n");
            outf("    br add <addr>\n");
            outf("    br rm <addr>\n");
            outf("Example:\n");
            outf("    br add 0x10000000\n");
            outf("    br rm 0x10000000\n");
        }
    } else {
        outf("Use 'help' to print list of the supported commands\n");
    }
}

void CmdParserService::splitLine(char *str, AttributeType *listArgs) {
    char *end = str;
    bool last = false;
    bool inside_string = false;
    char string_marker;
    while (*str) {
        if (*end == '\0') {
            last = true;
        } else if (*end == ' ' && !inside_string) {
            *end = '\0';
            while (*(end + 1) == ' ') {
                *(++end) = '\0';
            }
        } else  if (!inside_string && (*end == '"' || *end == '\'')) {
            inside_string = true;
            string_marker = *end;
        } else if (inside_string && string_marker == *end) {
            inside_string = false;
        }

        if (*end == '\0') {
            AttributeType item;
            if ((str[0] >= '0' && str[0] <= '9')
             || (str[0] == '[') || (str[0] == '"') || (str[0] == '\'')) {
                item.from_config(str);
            } else {
                item.make_string(str);
            }
            listArgs->add_to_list(&item);
            if (!last) {
                splitLine(end + 1, listArgs);
            }
            break;
        }
        ++end;
    }
}

void CmdParserService::readCSR(AttributeType *listArgs) {
    uint64_t csr = 0;
    uint64_t addr = csr_socaddr(&(*listArgs)[1]);

    if (addr == static_cast<uint64_t>(-1)) {
        outf("Unknown CSR '%s'\n", (*listArgs)[1].to_string());
        return;
    }

    itap_->read(addr, 8, reinterpret_cast<uint8_t *>(&csr));
    outf("CSR[%03x] => %016" RV_PRI64 "x\n", 
        static_cast<uint32_t>((addr >> 4) & 0xfff), csr);
   
    if (!(*listArgs)[1].is_string()) {
        return;
    }
    if (strcmp((*listArgs)[1].to_string(), "MCPUID") == 0) {
        static const char *MCPUID_BASE[4] = {
            "RV32I", "RV32E", "RV64I", "RV128I"
        };
        outf("    Base: %s", MCPUID_BASE[(csr >> 62) & 0x3]);
        // total 26 extensions
        char extenstion[2] = {0};
        for (int i = 0; i < 26; i++) {
            if (csr & (1LL << i)) {
                extenstion[0] = 'A' + i;
                if (extenstion[0] == 'I') {
                    continue;
                }
                outf("%s", extenstion);
            }
        }
        outf("\n");
    } else if (strcmp((*listArgs)[1].to_string(), "MTIME") == 0) {
        outf("    Clock: %" RV_PRI64 "d; ms: %.1f\n", csr, 
                            static_cast<double>(csr)/60000.0);
    }
}

void CmdParserService::loadElf(AttributeType *listArgs) {
    AttributeType attrReset("MRESET");
    uint64_t mreset = 1;
    uint64_t addr = csr_socaddr(&attrReset);

    itap_->write(addr, 8, reinterpret_cast<uint8_t *>(&mreset));
    iloader_->loadFile((*listArgs)[1].to_string());

    mreset = 0;
    itap_->write(addr, 8, reinterpret_cast<uint8_t *>(&mreset));
}

void CmdParserService::writeCSR(AttributeType *listArgs) {
    uint64_t addr;
    uint64_t csr;
    addr = csr_socaddr(&(*listArgs)[1]);
    if (addr == static_cast<uint64_t>(-1)) {
        outf("Unknown CSR '%s'\n", (*listArgs)[1].to_string());
        return;
    }
        
    csr = (*listArgs)[2].to_uint64();
    itap_->write(addr, 8, reinterpret_cast<uint8_t *>(&csr));
}

void CmdParserService::readMem(AttributeType *listArgs) {
    uint64_t addr_start, addr_end, inv_i;
    uint64_t addr = (*listArgs)[1].to_uint64();
    int bytes = static_cast<int>((*listArgs)[2].to_uint64());
    if (bytes > tmpbuf_size_) {
        delete [] tmpbuf_;
        tmpbuf_size_ = bytes;
        tmpbuf_ = new uint8_t[tmpbuf_size_ + 1];
    }
    addr_start = addr & ~0xFll;
    addr_end = (addr + bytes + 15 )& ~0xFll;

    itap_->read(addr, bytes, tmpbuf_);
    for (uint64_t i = addr_start; i < addr_end; i++) {
        if ((i & 0xf) == 0) {
            outf("[%08x%08x]: ", static_cast<uint32_t>(i >> 32),
                                 static_cast<uint32_t>(i));
        }
        inv_i = (i & ~0xFll) | (0xfll - (i & 0xfll));
        if ((addr <= inv_i) && (inv_i < (addr + bytes))) {
            outf(" %02x", tmpbuf_[inv_i - addr]);
        } else {
            outf(" ..");
        }
        if ((i & 0xf) == 0xf) {
            outf("\n");
        }
    }
}

void CmdParserService::writeMem(AttributeType *listArgs) {
    uint64_t addr = (*listArgs)[1].to_uint64();
    uint64_t val = (*listArgs)[3].to_uint64();
    int bytes = static_cast<int>((*listArgs)[2].to_uint64());

    if ((*listArgs)[3].is_integer()) {
        reinterpret_cast<uint64_t *>(tmpbuf_)[0] = val;
    } else if ((*listArgs)[3].is_list()) {
        for (unsigned i = 0; i < (*listArgs)[3].size(); i++) {
            val = (*listArgs)[3][i].to_uint64();
            reinterpret_cast<uint64_t *>(tmpbuf_)[i] = val;
        }
    } else {
        outf("Wrong write format\n");
        return;
    }
    itap_->write(addr, bytes, tmpbuf_);
}

void CmdParserService::memDump(AttributeType *listArgs) {
    const char *filename = (*listArgs)[3].to_string();
    FILE *fd = fopen(filename, "w");
    if (fd == NULL) {
        outf("Can't open '%s' file", filename);
        return;
    }
    uint64_t addr = (*listArgs)[1].to_uint64();
    int len = static_cast<int>((*listArgs)[2].to_uint64());
    uint8_t *dumpbuf = tmpbuf_;
    if (len > tmpbuf_size_) {
        dumpbuf = new uint8_t[len];
    }

    itap_->read(addr, len, dumpbuf);

    if (listArgs->size() == 5
        && strcmp((*listArgs)[4].to_string(), "hex") == 0) {
        char t1[256];
        int t1_cnt = 0;
        for (int i = 0; i < len; i++) {
            t1_cnt += sprintf(&t1[t1_cnt], "%02x", 
                dumpbuf[(i & ~0xf) | (0xf - (i & 0xf))]);
            if ((i & 0xf) == 0xf) {
                t1_cnt += sprintf(&t1[t1_cnt], "\n");
                fwrite(t1, t1_cnt, 1, fd);
                t1_cnt = 0;
            }
        }
        if (len & 0xf) {
            fwrite(t1, t1_cnt, 1, fd);
        }
    } else {
        fwrite(dumpbuf, len, 1, fd);
    }
    fclose(fd);
    if (dumpbuf != tmpbuf_) {
        delete [] dumpbuf;
    }
}

void CmdParserService::halt(AttributeType *listArgs) {
    uint64_t addr = DSU_CTRL_BASE_ADDRESS;
    DsuRunControlRegType ctrl;
    ctrl.val = 0x0;
    ctrl.bits.core_id = 0;
    ctrl.bits.halt    = 1;
    itap_->write(addr, 8, ctrl.buf);
}

void CmdParserService::run(AttributeType *listArgs) {
    uint64_t addr = DSU_CTRL_BASE_ADDRESS;
    DsuRunControlRegType ctrl;
    ctrl.val = 0x0;
    if (listArgs->size() == 1) {
        itap_->write(addr, 8, ctrl.buf);
    } else if (listArgs->size() == 2) {
        // Write number of steps:
        ctrl.val = (*listArgs)[1].to_uint64();
        itap_->write(addr + 8, 8, ctrl.buf);
        // Run execution:
        ctrl.val = 0;
        ctrl.bits.stepping = 1;
        itap_->write(addr, 8, ctrl.buf);
    }
}

void CmdParserService::regs(AttributeType *listArgs) {
    uint64_t addr = DSU_CTRL_BASE_ADDRESS + 64*8;   // CPU register array
    uint64_t regs[128] = {0};
    for (unsigned i = 1; i < regNames_.size(); i++) {
        itap_->read(addr + 8*i, 8, reinterpret_cast<uint8_t *>(&regs[i]));
    }
    outf("ra: %016" RV_PRI64 "x    \n", regs[getRegIDx("ra")]);

    outf("                        ", NULL);
    outf("s0:  %016" RV_PRI64 "x   ", regs[getRegIDx("s0")]);
    outf("a0:  %016" RV_PRI64 "x   \n", regs[getRegIDx("a0")]);

    outf("sp: %016" RV_PRI64 "x    ", regs[getRegIDx("sp")]);
    outf("s1:  %016" RV_PRI64 "x   ", regs[getRegIDx("s1")]);
    outf("a1:  %016" RV_PRI64 "x   \n", regs[getRegIDx("a1")]);

    outf("gp: %016" RV_PRI64 "x    ", regs[getRegIDx("gp")]);
    outf("s2:  %016" RV_PRI64 "x   ", regs[getRegIDx("s2")]);
    outf("a2:  %016" RV_PRI64 "x   \n", regs[getRegIDx("a2")]);

    outf("tp: %016" RV_PRI64 "x    ", regs[getRegIDx("tp")]);
    outf("s3:  %016" RV_PRI64 "x   ", regs[getRegIDx("s3")]);
    outf("a3:  %016" RV_PRI64 "x   \n", regs[getRegIDx("a3")]);

    outf("                        ", NULL);
    outf("s4:  %016" RV_PRI64 "x   ", regs[getRegIDx("s4")]);
    outf("a4:  %016" RV_PRI64 "x   \n", regs[getRegIDx("a4")]);

    outf("t0: %016" RV_PRI64 "x    ", regs[getRegIDx("t0")]);
    outf("s5:  %016" RV_PRI64 "x   ", regs[getRegIDx("s5")]);
    outf("a5:  %016" RV_PRI64 "x   \n", regs[getRegIDx("a5")]);

    outf("t1: %016" RV_PRI64 "x    ", regs[getRegIDx("t1")]);
    outf("s6:  %016" RV_PRI64 "x   ", regs[getRegIDx("s6")]);
    outf("a6:  %016" RV_PRI64 "x   \n", regs[getRegIDx("a6")]);

    outf("t2: %016" RV_PRI64 "x    ", regs[getRegIDx("t2")]);
    outf("s7:  %016" RV_PRI64 "x   ", regs[getRegIDx("s7")]);
    outf("a7:  %016" RV_PRI64 "x   \n", regs[getRegIDx("a7")]);
    
    outf("t3: %016" RV_PRI64 "x    ", regs[getRegIDx("t3")]);
    outf("s8:  %016" RV_PRI64 "x   \n", regs[getRegIDx("s8")]);

    outf("t4: %016" RV_PRI64 "x    ", regs[getRegIDx("t4")]);
    outf("s9:  %016" RV_PRI64 "x   \n", regs[getRegIDx("s9")]);

    outf("t5: %016" RV_PRI64 "x    ", regs[getRegIDx("t5")]);
    outf("s10: %016" RV_PRI64 "x   ", regs[getRegIDx("s10")]);
    outf("pc:  %016" RV_PRI64 "x  \n", regs[getRegIDx("pc")]);

    outf("t6: %016" RV_PRI64 "x    ", regs[getRegIDx("t6")]);
    outf("s11: %016" RV_PRI64 "x   ", regs[getRegIDx("s11")]);
    outf("npc: %016" RV_PRI64 "x   \n", regs[getRegIDx("npc")]);
}

void CmdParserService::br(AttributeType *listArgs) {
    uint64_t value = (*listArgs)[2].to_uint64();
    if (strcmp((*listArgs)[1].to_string(), "add") == 0) {
        // CPU add_breakpoint register
        uint64_t dsu_off = DSU_CTRL_BASE_ADDRESS + 16;
        itap_->write(dsu_off, 8, reinterpret_cast<uint8_t *>(&value));
    } else if (strcmp((*listArgs)[1].to_string(), "rm") == 0) {
        uint64_t dsu_off = DSU_CTRL_BASE_ADDRESS + 24;
        itap_->write(dsu_off, 8, reinterpret_cast<uint8_t *>(&value));
    }
}

unsigned CmdParserService::getRegIDx(const char *name) {
    for (unsigned i = 0; i < regNames_.size(); i++) {
        if (strcmp(name, regNames_[i][REG_Name].to_string()) == 0) {
            return static_cast<unsigned>(regNames_[i][REG_IDx].to_uint64());
        }
    }
    return 0;
}

int CmdParserService::outf(const char *fmt, ...) {
    if (outbuf_cnt_ > (outbuf_size_ - 128)) {
        char *t = new char [2*outbuf_size_];
        memcpy(t, outbuf_, outbuf_size_);
        delete [] outbuf_;
        outbuf_size_ *= 2;
        outbuf_ = t;
    }
    va_list arg;
    va_start(arg, fmt);
    outbuf_cnt_ += vsprintf(&outbuf_[outbuf_cnt_], fmt, arg);
    va_end(arg);
    return outbuf_cnt_;
}

void CmdParserService::addToHistory(const char *cmd) {
    unsigned found = history_.size();
    for (unsigned i = 0; i < history_.size(); i++) {
        if (strcmp(cmd, history_[i].to_string()) == 0) {
            found = i;
            break;
        }
    }
    if (found  ==  history_.size()) {
        AttributeType new_cmd(cmd);
        history_.add_to_list(&new_cmd);

        unsigned min_size = static_cast<unsigned>(history_size_.to_int64());
        if (history_.size() >= 2*min_size) {
            history_.trim_list(0, min_size);
        }
    } else if (found < (history_.size() - 1)) {
        history_.swap_list_item(found, history_.size() - 1);
    }
    history_idx_ = history_.size();
}

}  // namespace debugger
