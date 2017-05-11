/* 
 * File:   Model.cpp
 * Author: nguyentran
 * 
 * Created on March 22, 2013, 2:26 PM
 */

#include "Model.h"
#include "Population.h"
#include "HelperFunction.h"
#include "Config.h"
#include "Person.h"
#include "Random.h"
#include "ModelDataCollector.h"
#include "BirthdayEvent.h"
#include "ProgressToClinicalEvent.h"
#include "EndClinicalDueToDrugResistanceEvent.h"
#include "TestTreatmentFailureEvent.h"
#include "UpdateWhenDrugIsPresentEvent.h"
#include "EndClinicalByNoTreatmentEvent.h"
#include "EndClinicalEvent.h"
#include "Drug.h"
#include "ImmuneSystem.h"
#include "SingleHostClonalParasitePopulations.h"
#include "MatureGametocyteEvent.h"
#include "MoveParasiteToBloodEvent.h"
#include "UpdateEveryKDaysEvent.h"
#include "Reporter.h"
#include "BittingLevelGenerator.h"
#include "CirculateToTargetLocationNextDayEvent.h"
#include "ReturnToResidenceEvent.h"
#include "ClonalParasitePopulation.h"
#include "NonInfantImmuneComponent.h"
#include "InfantImmuneComponent.h"
#include "SwitchImmuneComponentEvent.h"
#include "FarmReporter.h"
#include "FarmReporter.h"
#include "ExternalPopulation.h"
#include "MoveToExternalPopulationEvent.h"
#include "ReturnToNormalPopulationEvent.h"
#include "TMEScheduler.h"
#include "ImportationPeriodicallyEvent.h"
#include "ImportationEvent.h"

Model* Model::MODEL = NULL;
Config* Model::CONFIG = NULL;
Random* Model::RANDOM = NULL;
Scheduler* Model::SCHEDULER = NULL;
TMEScheduler* Model::TME_SCHEDULER = NULL;
ModelDataCollector* Model::DATA_COLLECTOR = NULL;
Population* Model::POPULATION = NULL;
Population* Model::EXTERNAL_POPULATION = NULL;

Model::Model(const int &object_pool_size) {
    initialize_object_pool(object_pool_size);
    random_ = new Random(this);
    config_ = new Config(this);
    scheduler_ = new Scheduler(this);
    tme_scheduler_ = new TMEScheduler(this);
    population_ = new Population(this);
    external_population_ = new ExternalPopulation(this);
    data_collector_ = new ModelDataCollector(this);

    progress_to_clinical_update_function_ = new ClinicalUpdateFunction(this);
    immunity_clearance_update_function_ = new ImmunityClearanceUpdateFunction(this);
    having_drug_update_function_ = new ImmunityClearanceUpdateFunction(this);
    clinical_update_function_ = new ImmunityClearanceUpdateFunction(this);

    MODEL = this;
    CONFIG = config_;
    SCHEDULER = scheduler_;
    TME_SCHEDULER = tme_scheduler_;
    RANDOM = random_;
    DATA_COLLECTOR = data_collector_;
    POPULATION = population_;
    EXTERNAL_POPULATION = external_population_;

    reporters_ = std::vector<Reporter*>();

    initial_seed_number_ = -1;
    config_filename_ = "config.yml";
    tme_filename_ = "tme.txt";
    override_parameter_filename_ = "";
    override_parameter_line_number_ = -1;
    gui_type_ = -1;
    is_farm_output_ = false;
}

Model::Model(const Model& orig) {

}

Model::~Model() {
    //    std::cout << "Model Release" << std::endl;
    DeletePointer<ClinicalUpdateFunction>(progress_to_clinical_update_function_);
    DeletePointer<ImmunityClearanceUpdateFunction>(immunity_clearance_update_function_);
    DeletePointer<ImmunityClearanceUpdateFunction>(clinical_update_function_);

    DeletePointer<Population>(population_);
    DeletePointer<ExternalPopulation>(external_population_);
    DeletePointer<Scheduler>(scheduler_);
    DeletePointer<ModelDataCollector>(data_collector_);

    DeletePointer<Config>(config_);
    DeletePointer<Random>(random_);

    BOOST_FOREACH(Reporter* reporter, reporters_) {
        DeletePointer<Reporter>(reporter);
    }
    reporters_.clear();

    MODEL = NULL;
    CONFIG = NULL;
    SCHEDULER = NULL;
    TME_SCHEDULER = NULL;
    RANDOM = NULL;
    DATA_COLLECTOR = NULL;
    POPULATION = NULL;
    EXTERNAL_POPULATION = NULL;

    release_object_pool();
}

void Model::initialize() {
    //Initialize Random Seed
    random_->initialize(initial_seed_number_);

    //Read input file
    config_->read_from_file(config_filename_);

    // modify parameters    
    //modify parameters && update config
    config_->override_parameters(override_parameter_filename_, override_parameter_line_number_);

    //add reporter here    
    if (gui_type_ == -1) {
        if (is_farm_output_) {
            add_reporter(Reporter::MakeReport(Reporter::FARM));
        } else {
            add_reporter(Reporter::MakeReport(Reporter::BURNIN_FARM_REPORTER));
            add_reporter(Reporter::MakeReport(Reporter::BURNIN_MONTHLY_REPORTER));
        }
    } else {
        add_reporter(Reporter::MakeReport(Reporter::GUI));
    }

    if (override_parameter_line_number_ != -1) {
        //        add_reporter(Reporter::MakeReport(Reporter::YEARLY_REPORTER_V1));
    }


    //initialize reporters

    BOOST_FOREACH(Reporter* reporter, reporters_) {
        reporter->initialize();
    }



    //initialize scheduler
    scheduler_->initialize(config_->total_time() + 4000);

    tme_scheduler_->initialize();
    if (CONFIG->tme_info().tme_starting_day != 0) {
        tme_scheduler_->read_csv(tme_filename_);
    }

    //initialize data_collector
    data_collector_->initialize();

    //initialize Population
    population_->initialize();

    //initialize infected_cases
    population_->introduce_initial_cases();

    //initialize external population
    external_population_->initialize();


    //schedule for some special or periodic events

    for (int i = 0; i < CONFIG->importation_parasite_periodically_info().size(); i++) {
        ImportationPeriodicallyEvent::schedule_event(SCHEDULER, CONFIG->importation_parasite_periodically_info()[i].location, CONFIG->importation_parasite_periodically_info()[i].duration, CONFIG->importation_parasite_periodically_info()[i].parasite_type_id, CONFIG->importation_parasite_periodically_info()[i].number);
    }

    for (int i = 0; i < CONFIG->importation_parasite_info().size(); i++) {
        ImportationEvent::schedule_event(SCHEDULER, CONFIG->importation_parasite_info()[i].location, CONFIG->importation_parasite_info()[i].time, CONFIG->importation_parasite_info()[i].parasite_type_id, CONFIG->importation_parasite_info()[i].number);
    }
}

void Model::initialize_object_pool(const int& size) {
    BirthdayEvent::InitializeObjectPool(size);
    ProgressToClinicalEvent::InitializeObjectPool(size);
    EndClinicalDueToDrugResistanceEvent::InitializeObjectPool(size);
    TestTreatmentFailureEvent::InitializeObjectPool(size);
    UpdateWhenDrugIsPresentEvent::InitializeObjectPool(size);
    EndClinicalEvent::InitializeObjectPool(size);
    EndClinicalByNoTreatmentEvent::InitializeObjectPool(size);
    MatureGametocyteEvent::InitializeObjectPool(size);
    MoveParasiteToBloodEvent::InitializeObjectPool(size);
    UpdateEveryKDaysEvent::InitializeObjectPool(size);
    CirculateToTargetLocationNextDayEvent::InitializeObjectPool(size);
    ReturnToResidenceEvent::InitializeObjectPool(size);
    SwitchImmuneComponentEvent::InitializeObjectPool(size);
    MoveToExternalPopulationEvent::InitializeObjectPool(size);
    ReturnToNormalPopulationEvent::InitializeObjectPool(size);
    ImportationPeriodicallyEvent::InitializeObjectPool(size);
    ImportationEvent::InitializeObjectPool(size);

    ClonalParasitePopulation::InitializeObjectPool(size);
    SingleHostClonalParasitePopulations::InitializeObjectPool();

    Drug::InitializeObjectPool(size);
    DrugsInBlood::InitializeObjectPool();

    InfantImmuneComponent::InitializeObjectPool(size);
    NonInfantImmuneComponent::InitializeObjectPool(size);
    ImmuneSystem::InitializeObjectPool();
    Person::InitializeObjectPool(size);
}

void Model::release_object_pool() {
    //    std::cout << "Release object pool" << std::endl;
#ifdef UNITTEST

#else
    Person::ReleaseObjectPool();

    ImmuneSystem::ReleaseObjectPool();
    NonInfantImmuneComponent::ReleaseObjectPool();
    InfantImmuneComponent::ReleaseObjectPool();

    DrugsInBlood::ReleaseObjectPool();
    Drug::ReleaseObjectPool();

    SingleHostClonalParasitePopulations::ReleaseObjectPool();
    ClonalParasitePopulation::ReleaseObjectPool();

    ImportationEvent::ReleaseObjectPool();
    ImportationPeriodicallyEvent::ReleaseObjectPool();
    ReturnToNormalPopulationEvent::ReleaseObjectPool();
    MoveToExternalPopulationEvent::ReleaseObjectPool();
    SwitchImmuneComponentEvent::ReleaseObjectPool();
    ReturnToResidenceEvent::ReleaseObjectPool();
    CirculateToTargetLocationNextDayEvent::ReleaseObjectPool();
    UpdateEveryKDaysEvent::ReleaseObjectPool();
    MoveParasiteToBloodEvent::ReleaseObjectPool();
    MatureGametocyteEvent::ReleaseObjectPool();
    EndClinicalByNoTreatmentEvent::ReleaseObjectPool();
    EndClinicalEvent::ReleaseObjectPool();
    UpdateWhenDrugIsPresentEvent::ReleaseObjectPool();
    TestTreatmentFailureEvent::ReleaseObjectPool();
    EndClinicalDueToDrugResistanceEvent::ReleaseObjectPool();
    ProgressToClinicalEvent::ReleaseObjectPool();
    BirthdayEvent::ReleaseObjectPool();
#endif
}

//TODO: model override Config from override file

void Model::run() {
    before_run();
    scheduler_->run();
    after_run();
}

void Model::before_run() {
    //    std::cout << "Seed:" << RANDOM->seed() << std::endl;

    BOOST_FOREACH(Reporter* reporter, reporters_) {
        reporter->before_run();
    }
}

void Model::after_run() {
    Model::DATA_COLLECTOR->update_after_run();

    BOOST_FOREACH(Reporter* reporter, reporters_) {
        reporter->after_run();
    }
}

void Model::perform_infection_event() {
    population_->perform_infection_event();
    external_population_->perform_infection_event();
}

void Model::report_end_of_time_step() {
    if (Model::SCHEDULER->current_time() % Model::CONFIG->report_frequency() == 0) {
        Model::DATA_COLLECTOR->perform_population_statistic();

        BOOST_FOREACH(Reporter* reporter, reporters_) {
            reporter->after_time_step();
        }
    }
    //     if (Model::SCHEDULER->current_time() >= 4000) {
    //        std::cout << "end" << std::endl;
    //    }
}

void Model::report_begin_of_time_step() {

    BOOST_FOREACH(Reporter* reporter, reporters_) {
        reporter->begin_time_step();
    }
    //    if (Model::SCHEDULER->current_time() >= 4000) {
    //        std::cout << "begin" << std::endl;
    //    }
}

void Model::add_reporter(Reporter* reporter) {
    reporters_.push_back(reporter);
    reporter->set_model(this);
}