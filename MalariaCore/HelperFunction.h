/* 
 * File:   HelperFunction.h
 * Author: nguyentran
 *
 * Created on April 16, 2013, 10:24 AM
 */

#ifndef HELPERFUNCTION_H
#define	HELPERFUNCTION_H

#include <string>
#include <sstream>
#include <vector>

template <typename T>
void DeletePointer(T*& p) {
    if (p != NULL) {
        delete p;
        p = NULL;
    }
}

template <typename T>
std::string NumberToString(T Number) {
    std::ostringstream ss;
    ss << Number;
    return ss.str();
}



#endif	/* HELPERFUNCTION_H */

