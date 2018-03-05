/* 
 * File:   ConsoleReporter.h
 * Author: Merlin
 *
 * Created on August 1, 2013, 12:15 PM
 */

#ifndef CONSOLEREPORTER_H
#define	CONSOLEREPORTER_H

#include "Reporter.h"

class ConsoleReporter : public Reporter {
    DISALLOW_COPY_AND_ASSIGN_(ConsoleReporter)
public:
    ConsoleReporter();
    //    ConsoleReporter(const ConsoleReporter& orig);
    virtual ~ConsoleReporter();

    virtual void initialize();
    virtual void before_run();
    virtual void after_run();
    virtual void begin_time_step();
    virtual void after_time_step();

private:

};

#endif	/* CONSOLEREPORTER_H */

