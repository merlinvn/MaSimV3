/* 
 * File:   ImmuneSystem.h
 * Author: nguyentran
 *
 * Created on May 27, 2013, 11:56 AM
 */

#ifndef IMMUNESYSTEM_H
#define	IMMUNESYSTEM_H
#include "PropertyMacro.h"
#include <vector>
#include "TypeDef.h"
#include "ObjectPool.h"

class Model;
class Person;
class ImmuneComponent;
class Genotype;
class Config;


//typedef std::vector<ImmuneComponent*> ImmuneComponentPtrVector;

class ImmuneSystem {
    OBJECTPOOL(ImmuneSystem)
    DISALLOW_COPY_AND_ASSIGN_(ImmuneSystem)
    POINTER_PROPERTY(Person, person);
    VIRTUAL_PROPERTY_REF(bool, increase);

//    POINTER_PROPERTY_REF(ImmuneComponentPtrVector, immune_components);
    
    POINTER_PROPERTY_HEADER(ImmuneComponent, immune_component);

public:
    ImmuneSystem(Person* p = NULL);
    virtual ~ImmuneSystem();

    static void Initialize();

    
    //    virtual void clear();

    virtual void draw_random_immune();

    virtual void update();

    virtual double get_lastest_immune_value() const;
    virtual void set_latest_immune_value(double value);

    virtual double get_current_value() const;

    virtual double get_parasite_size_after_t_days(const int& duration, const double &originalSize, const double &fitness) const;

    virtual double get_clinical_progression_probability() const;


private:

};

#endif	/* IMMUNESYSTEM_H */

