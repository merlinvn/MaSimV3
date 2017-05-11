/* 
 * File:   Population.h
 * Author: nguyentran
 *
 * Created on April 15, 2013, 10:49 AM
 */

#ifndef POPULATION_H
#define	POPULATION_H




#include "PropertyMacro.h"
#include "TypeDef.h"
#include "Person.h"
#include "PersonIndex.h"
#include "Dispatcher.h"
#include <boost/foreach.hpp>

//#include "PersonIndexByLocationStateAgeClass.h"


//class Person;
class Model;
class PersonIndexAll;
class PersonIndexByLocationStateAgeClass;
class PersonIndexByLocationBittingLevel;

/**
 * Population will manage the life cycle of Person object
 * it will release/delete all person object when it is deleted
 * all person index will do nothing
 * 
 */
class Population : public Dispatcher {
    DISALLOW_COPY_AND_ASSIGN_(Population)
    POINTER_PROPERTY(Model, model);

    POINTER_PROPERTY(PersonIndexPtrList, person_index_list);
    POINTER_PROPERTY(PersonIndexAll, all_persons);

    PROPERTY_REF(std::vector< std::vector <double> >, current_force_of_infection_by_location_parasite_type);
    PROPERTY_REF(std::vector< std::vector <double> >, interupted_feeding_force_of_infection_by_location_parasite_type);
    PROPERTY_REF(std::vector< std::vector< std::vector <double> > >, force_of_infection_for7days_by_location_parasite_type);


public:
    Population(Model* model = NULL);
    virtual ~Population();

    /**
     * This function will add Person pointer to all of the person indexes
     * @param person
     */
    virtual void add_person(Person* person);

    //just remove from index, no delete pointer
    virtual void remove_person(Person* person);


    /**
     * This function removes person pointer out of all of the person indexes
     * This will also delete the @person out of memory
     * @param person
     */
    virtual void remove_dead_person(Person* person);

    /**
     * Notify change of a particular person's property to all person indexes
     * @param p
     * @param property
     * @param oldValue
     * @param newValue
     */
    virtual void notify_change(Person* p, const Person::PersonProperties& property, const void* oldValue, const void* newValue);

    /**
     * Return the number of individuals in the population
     * If the input location is -1, return total size
     * @param location
     */
    virtual int size(const int &location = -1, const int &age_class = -1);
    virtual int size(const int &location, const Person::HostStates& hs, const int &age_class);

    virtual void perform_infection_event();

    virtual void initialize();

    void introduce_initial_cases();

    template <typename T>
    T* get_person_index();

    void introduce_parasite(const int& location, IntGenotype* parasite_type, const int& num_of_infections);

    void initial_infection(Person* p, IntGenotype* parasite_type);

    virtual void notify_change_in_force_of_infection(const int& location, const int& parasite_type_id, const double& relative_force_of_infection);

    virtual void update();

    void perform_birth_event();
    void perform_death_event();

    void give_1_birth(const int& location);

    void clear_all_dead_state_individual();

    void perform_circulation_event();
    
    void perform_moving_to_external_population_event();

    void perform_circulation_for_1_location(const int & from_location, const int& target_location, const int& number_of_circulation, std::vector<Person*> &today_circulations);

    bool has_0_case();

    void initialize_person_indices();
    
    void perform_interupted_feeding_recombination();

private:

};

template<typename T>
T* Population::get_person_index() {

    BOOST_FOREACH(PersonIndex* person_index, *person_index_list_) {
        if (dynamic_cast<T*> (person_index) != NULL) {
            T* pi = dynamic_cast<T*> (person_index);
            return pi;
        }
    }
    return NULL;
}

#endif	/* POPULATION_H */

