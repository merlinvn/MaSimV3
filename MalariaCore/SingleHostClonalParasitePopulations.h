/* 
 * File:   ParasitePopulation.h
 * Author: Merlin
 *
 * Created on July 11, 2013, 1:53 PM
 */

#ifndef SINGLEHOSTCLONALPARASITEPOPULATIONS_H
#define	SINGLEHOSTCLONALPARASITEPOPULATIONS_H

#include "PropertyMacro.h"
#include "ObjectPool.h"
#include "TypeDef.h"
#include <vector>

class ClonalParasitePopulation;
class Person;
class DrugType;
class DrugsInBlood;

class SingleHostClonalParasitePopulations {
    OBJECTPOOL(SingleHostClonalParasitePopulations)
    DISALLOW_COPY_AND_ASSIGN_(SingleHostClonalParasitePopulations)

    POINTER_PROPERTY(Person, person)

    POINTER_PROPERTY(std::vector<ClonalParasitePopulation*>, parasites)
    POINTER_PROPERTY(DoubleVector, relative_effective_parasite_density)

    PROPERTY_REF(double, log10_total_relative_density);

public:
    SingleHostClonalParasitePopulations(Person* person = NULL);
    //    ParasitePopulation(const ParasitePopulation& orig);
    virtual ~SingleHostClonalParasitePopulations();

    void init();

    virtual int size();

    virtual void add(ClonalParasitePopulation* blood_parasite);

    virtual void remove(ClonalParasitePopulation* blood_parasite);
    virtual void remove(const int& index);

    virtual void add_all_infection_force();
    virtual void remove_all_infection_force();

    virtual void change_all_infection_force(const double& sign);

    virtual double get_log10_total_relative_density();

    virtual int latest_update_time() const;

    virtual bool contain(ClonalParasitePopulation* blood_parasite);

    void change_all_parasite_update_function(ParasiteDensityUpdateFunction* from, ParasiteDensityUpdateFunction* to);

    void active_astermisinin_on_gametocyte(DrugType* dt);

    void deactive_astermisinin_on_gametocyte();

    void update();

    void clear_cured_parasites();

    void clear();

    void update_by_drugs(DrugsInBlood* drugs_in_blood);

    bool has_detectable_parasite();

    void get_parasites_profiles(std::vector<double>& parasite_density, double& log10_total_relative_density);

    void update_relative_effective_parasite_density_using_free_recombination();
    
    void update_relative_effective_parasite_density_without_free_recombination();
    
    bool isGameticytaemic();
private:

};

#endif	/* SINGLEHOSTCLONALPARASITEPOPULATIONS_H */

