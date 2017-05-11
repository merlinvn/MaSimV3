/* 
 * File:   IndividualsFileReporter.cpp
 * Author: Merlin
 * 
 * Created on November 7, 2014, 11:06 AM
 */

#include "IndividualsFileReporter.h"
#include "Model.h"
#include "PersonIndexAll.h"
#include "Person.h"
#include "Population.h"
#include "Config.h"
#include "Scheduler.h"
#include "ClonalParasitePopulation.h"
#include "SingleHostClonalParasitePopulations.h"

IndividualsFileReporter::IndividualsFileReporter(const std::string& file_name) : file_name_(file_name) {
}

IndividualsFileReporter::IndividualsFileReporter(const IndividualsFileReporter& orig) {
}

IndividualsFileReporter::~IndividualsFileReporter() {
}

void IndividualsFileReporter::initialize() {


}

void IndividualsFileReporter::before_run() {
    fs_.open(file_name_.c_str(), std::fstream::out);
}

void IndividualsFileReporter::begin_time_step() {

}

void IndividualsFileReporter::after_time_step() {
    if (Model::SCHEDULER->current_time() % Model::CONFIG->report_frequency() == 0) {
        for (int i = 0; i < Model::POPULATION->all_persons()->vPerson().size(); i++) {
            Person* person = Model::POPULATION->all_persons()->vPerson()[i];
            double p_density = 0;
            if (person->all_clonal_parasite_populations()->parasites()->size() >= 1) {
                p_density = person->all_clonal_parasite_populations()->parasites()->at(0)->last_update_log10_parasite_density();
            } else {
                p_density = Model::CONFIG->log_parasite_density_level().log_parasite_density_cured;
            }
            fs_ << p_density << "\t";
        }
        fs_ << std::endl;
    }

}

void IndividualsFileReporter::after_run() {
    fs_.close();
}






