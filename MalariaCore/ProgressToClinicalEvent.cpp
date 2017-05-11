/* 
 * File:   ProgressToClinicalEvent.cpp
 * Author: Merlin
 * 
 * Created on July 30, 2013, 2:36 PM
 */

#include "ProgressToClinicalEvent.h"
#include "Person.h"
#include "Scheduler.h"
#include "SingleHostClonalParasitePopulations.h"
#include "Population.h"
#include "Model.h"
#include "Config.h"
#include "Strategy.h"
#include "SCTherapy.h"
#include "ClonalParasitePopulation.h"
#include "Random.h"
#include "ModelDataCollector.h"

OBJECTPOOL_IMPL(ProgressToClinicalEvent)

ProgressToClinicalEvent::ProgressToClinicalEvent() {
}

ProgressToClinicalEvent::ProgressToClinicalEvent(const ProgressToClinicalEvent& orig) {
}

ProgressToClinicalEvent::~ProgressToClinicalEvent() {
}

void ProgressToClinicalEvent::execute() {
    Person* person = (Person*) dispatcher();
    if (person->all_clonal_parasite_populations()->size() == 0) {
        //parasites might be cleaned by immune system or other things else
        return;
    }

    //if the clinical_caused_parasite eventually removed then do nothing
    if (!person->all_clonal_parasite_populations()->contain(clinical_caused_parasite_)) {

        return;
    }

    if (person->host_state() == Person::CLINICAL) {
        clinical_caused_parasite_->set_update_function(Model::MODEL->immunity_clearance_update_function());
        return;
    }


    //    Model* model = person->population()->model();
    //    Random* random = model->random();
    //    Config* config = model->config();

    double density = Model::RANDOM->random_uniform_double(Model::CONFIG->log_parasite_density_level().log_parasite_density_clinical_from, Model::CONFIG->log_parasite_density_level().log_parasite_density_clinical_to);

    clinical_caused_parasite_->set_last_update_log10_parasite_density(density);

    // Person change state to Clinical
    person->set_host_state(Person::CLINICAL);

    //this event affect other parasites in population
    //only the parasite that will go to clinical will be change to noneUpdate function, 
    //P go to clearance will not be change
    //cancel all other progress to clinical events except current
    person->cancel_all_other_progress_to_clinical_events_except(this);

    person->change_all_parasite_update_function(Model::MODEL->progress_to_clinical_update_function(), Model::MODEL->immunity_clearance_update_function());
    clinical_caused_parasite_->set_update_function(Model::MODEL->clinical_update_function());

    //Statistic collect cumulative clinical episodes
    Model::DATA_COLLECTOR->collect_1_clinical_episode(person->location(), person->age(), person->age_class());

    double P = Model::RANDOM->random_flat(0.0, 1.0);
    double P_treatment = (Model::MODEL->scheduler()->current_time() >= Model::CONFIG->start_treatment_day()) ? Model::CONFIG->p_treatment() : -1;
    if (P <= P_treatment) {
        Therapy* therapy = Model::CONFIG->strategy()->get_therapy();
        person->receive_therapy(therapy, clinical_caused_parasite_);
        //Statistic increase today treatments
        Model::DATA_COLLECTOR->record_1_treatment(person->location(), person->age(), therapy->id());

        clinical_caused_parasite_->set_update_function(Model::MODEL->having_drug_update_function());

        // calculate EAMU
        Model::DATA_COLLECTOR->record_AMU_AFU(person, therapy, clinical_caused_parasite_);
        //        calculateEAMU(therapy);
        //

        // death is 90% lower than no treatment
        if (person->will_progress_to_death_when_recieve_treatment()) {

            //for each test treatment failure event inside individual 
            // record treatment failure (not tf)
            //            person->record_treatment_failure_for_test_treatment_failure_events();

            //no treatment routine
            receive_no_treatment_routine(person);

            person->cancel_all_events_except(NULL);
            person->set_host_state(Person::DEAD);
            Model::DATA_COLLECTOR->record_1_malaria_death(person->location(), person->age());
            Model::DATA_COLLECTOR->record_1_TF(person->location(), true);
            Model::DATA_COLLECTOR->record_1_treatment_failure_by_therapy(person->location(), person->age(), therapy->id());
            return;
        }
  
        person->schedule_update_by_drug_event(clinical_caused_parasite_);

        person->schedule_end_clinical_event(clinical_caused_parasite_);
        person->schedule_test_treatment_failure_event(clinical_caused_parasite_, Model::CONFIG->tf_testing_day(), false, therapy->id());


    } else {
        //not recieve treatment
        //Statistic store NTF
        Model::DATA_COLLECTOR->record_1_TF(person->location(), false);
        Model::DATA_COLLECTOR->record_1_non_treated_case(person->location(), person->age());

        receive_no_treatment_routine(person);
        if (person->host_state() == Person::DEAD) {
            Model::DATA_COLLECTOR->record_1_malaria_death(person->location(), person->age());
            return;
        }
        //
        //        //schedule for end of clinical event
        //        std::cout << "progress clinical event" << std::endl;

        person->schedule_end_clinical_by_no_treatment_event(clinical_caused_parasite_);
    }
}

void ProgressToClinicalEvent::schedule_event(Scheduler* scheduler, Person* p, ClonalParasitePopulation* clinical_caused_parasite, const int& time) {
    if (scheduler != NULL) {
        ProgressToClinicalEvent* e = new ProgressToClinicalEvent();
        e->set_dispatcher(p);
        e->set_clinical_caused_parasite(clinical_caused_parasite);
        e->set_executable(true);
        e->set_time(time);

        p->add(e);
        scheduler->schedule(e);
    }
}

void ProgressToClinicalEvent::receive_no_treatment_routine(Person* p) {
    if (p->will_progress_to_death_when_receive_no_treatment()) {
        p->cancel_all_events_except(NULL);
        p->set_host_state(Person::DEAD);
    }
}