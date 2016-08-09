/**
 * @file
 * @copyright  Copyright 2016 GNSS Sensor Ltd. All right reserved.
 * @author     Sergey Khabarov - sergeykhbr@gmail.com
 * @brief      Input console class declaration.
 */

#ifndef __DEBUGGER_CONSOLE_H__
#define __DEBUGGER_CONSOLE_H__

#include "iclass.h"
#include "iservice.h"
#include "ihap.h"
#include "ithread.h"
#include "iconsole.h"
#include "iserial.h"
#include "ikeylistener.h"
#include "irawlistener.h"
#include <string>

namespace debugger {

class ConsoleService : public IService,
                       public IThread,
                       public IConsole,
                       public IHap,
                       public IRawListener {
public:
    explicit ConsoleService(const char *name);
    virtual ~ConsoleService();

    /** IService interface */
    virtual void postinitService();
    virtual void predeleteService();

    /** IConsole interface */
    virtual void writeBuffer(const char *buf);
    virtual void writeCommand(const char *cmd);
    virtual void setCmdString(const char *buf);
    virtual int registerKeyListener(IFace *iface);
    virtual void enableLogFile(const char *filename);

    /** IHap */
    virtual void hapTriggered(EHapType type);

    /** ISerial */
    virtual void updateData(const char *buf, int buflen);

protected:
    /** IThread interface */
    virtual void busyLoop();

private:
    bool isData();
    int getData();
    void clearLine();

private:
    AttributeType isEnable_;
    AttributeType consumer_;
    AttributeType keyListeners_;
    AttributeType logFile_;
    AttributeType serial_;
    event_def config_done_;
    mutex_def mutexConsoleOutput_;
    IKeyListener *iconsumer_;
    char tmpbuf_[4096];
    std::string cmdLine_;
    std::string serial_input_;
    FILE *logfile_;
#if defined(_WIN32) || defined(__CYGWIN__)
#else
    struct termios original_settings_;
    int term_fd_;
#endif
};

DECLARE_CLASS(ConsoleService)

}  // namespace debugger

#endif  // __DEBUGGER_CONSOLE_H__
