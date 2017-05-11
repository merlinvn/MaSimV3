/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   BurinBurinFarmReporter.cpp
 * Author: Merlin
 * 
 * Created on April 11, 2017, 1:45 PM
 */

#include "BurninFarmReporter.h"
#include "Model.h"
#include "Random.h"
#include "ModelDataCollector.h"
#include "Config.h"
#include "Strategy.h"
#include "Therapy.h"
#include "SCTherapy.h"
#include "Population.h"
#include "MFTStrategy.h"
#include <boost/format.hpp>

BurninFarmReporter::BurninFarmReporter() {
}

BurninFarmReporter::BurninFarmReporter(const BurninFarmReporter& orig) {
}

BurninFarmReporter::~BurninFarmReporter() {
}

void BurninFarmReporter::initialize() {
}

void BurninFarmReporter::before_run() {
    std::cout << Model::RANDOM->seed() << std::endl;
}

void BurninFarmReporter::after_run() {
    //output parameter
    output_parameters();
    std::cout << "-1111\t";

    //output others indicators
    // EIR - 20x
    print_EIR_by_location();
    print_20x_by_location();
    // prevalance
    print_prevalence_by_location();

    // NTF
    print_ntf_by_location();

    print_fraction_of_positive_that_are_clinical_by_location();

    // mean immune
    print_mean_immune_by_location();
    // mean number of clones per positive Individual
    print_mean_number_of_clones_per_positive_individual();
    // LOI
    print_LOI();
    std::cout << "-1111\t";
    // resistance tracker
    print_resistance_tracker();

    std::cout << "-1111\t";
    // number of treatment by therapy
    print_treatments_by_therapy();

    std::cout << "-1111\t";



    // EAMU
    //    print_EAMU();


    // fraction of positive that are clinical





    // mean number of clones per positive Individual by age group
    print_mean_number_of_clones_per_positive_individual_by_age_group();

    // death by age class
    print_death_by_age_group();

    // number of clinical episode by age class
    print_number_of_clinical_episode_by_age_class();

    // prevalance by age class
    print_prevalence_by_age_class();
    // clinical percentage by age class
    print_fraction_of_positive_that_are_clinical_by_location_age_class();

    print_mean_immune_by_location_age_class();

    //print moi distribution
    print_moi_distribution();



    // current time
    std::cout << Model::SCHEDULER->current_time() << "\t";
    // utl
    print_total_utl();

    std::cout << Model::DATA_COLLECTOR->popsize_by_location()[0] << "\t";

    //
    //    print_popsize_by_age_class();

    //print phi value by 5-age-group
    print_fraction_of_positive_that_are_clinical_by_location_age_class_by_5();


    print_utl();

    std::cout << std::endl;
}

void BurninFarmReporter::begin_time_step() {
}

void BurninFarmReporter::after_time_step() {

}

void BurninFarmReporter::output_parameters() {
    std::cout << boost::format("%1%\t%2%\t")
            % Model::RANDOM->seed()
            % Model::CONFIG->number_of_locations();

    //intial population size
    for (int location = 0; location < Model::CONFIG->number_of_locations(); location++) {
        std::cout << Model::CONFIG->population_size_by_location()[location] << "\t";
    }
    //beta
    for (int location = 0; location < Model::CONFIG->number_of_locations(); location++) {
        std::cout << Model::CONFIG->beta()[location] << "\t";
    }

    std::cout << Model::CONFIG->p_treatment() << "\t";
    //    std::cout << Model::CONFIG->genotype_info().loci_vector[0].cost_of_resistance << "\t";
    std::cout << Model::CONFIG->genotype_info().loci_vector[0].alleles[1].daily_cost_of_resistance << "\t";
    std::cout << Model::CONFIG->genotype_info().loci_vector[1].alleles[4].daily_cost_of_resistance << "\t"; // print for mdr 2 copies daily cost of resistance
    std::cout << Model::CONFIG->drug_db()->drug_db().begin()->second->p_mutation() << "\t";

    for (int i = 0; i < Model::CONFIG->strategy()->therapy_list().size(); i++) {
        std::cout << Model::CONFIG->strategy()->therapy_list()[i]->id() << "\t";
    }

    if (dynamic_cast<MFTStrategy*> (Model::CONFIG->strategy()) != NULL) {
        MFTStrategy* mft = dynamic_cast<MFTStrategy*> (Model::CONFIG->strategy());
        for (int i = 0; i < mft->distribution().size(); i++) {
            std::cout << mft->distribution()[i] << "\t";
        }
    }

}

void BurninFarmReporter::print_ntf_by_location() {
    double total_time_in_years = (Model::SCHEDULER->current_time() - Model::CONFIG->start_collect_data_day()) / 365.0;
    for (int location = 0; location < Model::CONFIG->number_of_locations(); location++) {
        double location_discounted_NTF = Model::DATA_COLLECTOR->cumulative_discounted_NTF_by_location()[location] * 100 / (double) Model::DATA_COLLECTOR->popsize_by_location()[location];
        double NTF = Model::DATA_COLLECTOR->cumulative_NTF_by_location()[location] * 100 / (double) Model::DATA_COLLECTOR->popsize_by_location()[location];
        double NTF15_30 = Model::DATA_COLLECTOR->cumulative_NTF_15_30_by_location()[location]*100 / (double) Model::DATA_COLLECTOR->popsize_by_location()[location];

        location_discounted_NTF /= total_time_in_years;
        NTF /= total_time_in_years;

        NTF15_30 /= (Model::SCHEDULER->current_time() - Model::CONFIG->non_artemisinin_switching_day()) / 365.0;

        std::cout << location_discounted_NTF << "\t";
        std::cout << NTF << "\t";
        //        std::cout << NTF15_30 << "\t";
    }
}

void BurninFarmReporter::print_EIR_by_location() {
    for (int location = 0; location < Model::CONFIG->number_of_locations(); location++) {
        std::cout << Model::DATA_COLLECTOR->EIR_by_location()[location] << "\t";
    }
}

void BurninFarmReporter::print_20x_by_location() {
    for (int location = 0; location < Model::CONFIG->number_of_locations(); location++) {
        std::cout << Model::DATA_COLLECTOR->percentage_bites_on_top_20_by_location()[location] * 100 << "\t";
    }
}

void BurninFarmReporter::print_EAMU() {
    std::cout << Model::DATA_COLLECTOR->AMU_per_parasite_pop() << "\t";
    std::cout << Model::DATA_COLLECTOR->AMU_per_person() << "\t";
    std::cout << Model::DATA_COLLECTOR->AMU_for_clinical_caused_parasite() << "\t";
    std::cout << Model::DATA_COLLECTOR->AFU() << "\t";
}

void BurninFarmReporter::print_prevalence_by_location() {
    for (int location = 0; location < Model::CONFIG->number_of_locations(); location++) {
        double prevalence = 0.0;
        for (int i = 0; i < 10; i++) {
            prevalence += Model::DATA_COLLECTOR->last_10_blood_slide_prevalence_by_location()[location][i];
        }
        std::cout << prevalence / 10.0 << "\t";
    }
}

void BurninFarmReporter::print_fraction_of_positive_that_are_clinical_by_location() {
    for (int location = 0; location < Model::CONFIG->number_of_locations(); location++) {
        double f_clinical = 0.0;
        for (int i = 0; i < 10; i++) {
            f_clinical += Model::DATA_COLLECTOR->last_10_fraction_positive_that_are_clinical_by_location()[location][i];
        }
        std::cout << f_clinical / 10.0 << "\t";
    }

}

void BurninFarmReporter::print_mean_immune_by_location() {
    for (int location = 0; location < Model::CONFIG->number_of_locations(); location++) {
        std::cout << Model::DATA_COLLECTOR->total_immune_by_location()[location] / Model::DATA_COLLECTOR->popsize_by_location()[location] << "\t";
    }
}

void BurninFarmReporter::print_mean_number_of_clones_per_positive_individual() {
    for (int location = 0; location < Model::CONFIG->number_of_locations(); location++) {
        unsigned long totalClones = Model::DATA_COLLECTOR->total_parasite_population_by_location()[location];
        int count = Model::DATA_COLLECTOR->number_of_positive_by_location()[location];
        double r = (count == 0) ? 0 : (totalClones / (double) count);
        std::cout << r << "\t";
    }

}

void BurninFarmReporter::print_mean_number_of_clones_per_positive_individual_by_age_group() {
    for (int location = 0; location < Model::CONFIG->number_of_locations(); location++) {
        for (int ac = 0; ac < Model::CONFIG->number_of_age_classes(); ac++) {
            unsigned long totalClones = Model::DATA_COLLECTOR->total_parasite_population_by_location_age_group()[location][ac];
            int count = Model::DATA_COLLECTOR->number_of_positive_by_location_age_group()[location][ac];
            double r = (count == 0) ? 0 : (totalClones / (double) count);
            std::cout << r << "\t";
        }
    }
}

void BurninFarmReporter::print_death_by_age_group() {
    for (int location = 0; location < Model::CONFIG->number_of_locations(); location++) {
        for (int ac = 0; ac < Model::CONFIG->number_of_age_classes(); ac++) {

            std::cout << Model::DATA_COLLECTOR->number_of_death_by_location_age_group()[location][ac] << "\t";
        }
    }
}

void BurninFarmReporter::print_number_of_clinical_episode_by_age_class() {
    double total_time_in_years = (Model::SCHEDULER->current_time() - Model::CONFIG->start_collect_data_day()) / 365.0;
    for (int location = 0; location < Model::CONFIG->number_of_locations(); location++) {
        for (int ac = 0; ac < Model::CONFIG->number_of_age_classes(); ac++) {
            std::cout << (Model::DATA_COLLECTOR->cumulative_clinical_episodes_by_location_age_group()[location][ac] / (double) Model::DATA_COLLECTOR->popsize_by_location_age_class()[location][ac]) / total_time_in_years << "\t";
        }
    }
}

void BurninFarmReporter::print_prevalence_by_age_class() {
    for (int location = 0; location < Model::CONFIG->number_of_locations(); location++) {
        for (int ac = 0; ac < Model::CONFIG->number_of_age_classes(); ac++) {
            std::cout << Model::DATA_COLLECTOR->blood_slide_prevalence_by_location_age_group()[location][ac] << "\t";
        }
    }
}

void BurninFarmReporter::print_fraction_of_positive_that_are_clinical_by_location_age_class() {
    for (int location = 0; location < Model::CONFIG->number_of_locations(); location++) {
        for (int ac = 0; ac < Model::CONFIG->number_of_age_classes(); ac++) {

            double s = 0;
            for (int i = 0; i < 10; i++) {
                s += Model::DATA_COLLECTOR->last_10_fraction_positive_that_are_clinical_by_location_age_class()[location][ac][i];
            }
            std::cout << s / 10 << "\t";

            //            std::cout << Model::STATISTIC->number_of_clinical_by_location_age_group()[location][ac] / (double) Model::STATISTIC->number_of_positive_by_location_age_group()[location][ac] << "\t";
        }
    }
}

void BurninFarmReporter::print_fraction_of_positive_that_are_clinical_by_location_age_class_by_5() {
    for (int location = 0; location < Model::CONFIG->number_of_locations(); location++) {
        for (int ac = 0; ac < Model::CONFIG->number_of_age_classes(); ac++) {

            double s = 0;
            for (int i = 0; i < 10; i++) {
                s += Model::DATA_COLLECTOR->last_10_fraction_positive_that_are_clinical_by_location_age_class_by_5()[location][ac][i];
            }
            std::cout << s / 10 << "\t";

            //            std::cout << Model::STATISTIC->number_of_clinical_by_location_age_group()[location][ac] / (double) Model::STATISTIC->number_of_positive_by_location_age_group()[location][ac] << "\t";
        }
    }
}

void BurninFarmReporter::print_resistance_tracker() {
    for (int i = 0; i < Model::CONFIG->number_of_parasite_types(); i++) {
        std::cout << Model::DATA_COLLECTOR->resistance_tracker().parasite_population_count()[i] << "\t";
    }
}

void BurninFarmReporter::print_treatments_by_therapy() {
    for (int i = 0; i < Model::CONFIG->strategy()->therapy_list().size(); i++) {
        int t_id = Model::CONFIG->strategy()->therapy_list()[i]->id();
        int nTreaments = Model::DATA_COLLECTOR->number_of_treatments_with_therapy_ID()[t_id];
        int nSuccess = Model::DATA_COLLECTOR->number_of_treatments_success_with_therapy_ID()[t_id];
        int nFail = Model::DATA_COLLECTOR->number_of_treatments_fail_with_therapy_ID()[t_id];
        double pSuccess = (nTreaments == 0) ? 0 : nSuccess * 100.0 / (nSuccess + nFail);

        std::cout << t_id << "\t" << nFail + nSuccess << "\t" << nSuccess << "\t" << pSuccess << "\t";

    }
}

void BurninFarmReporter::print_total_utl() {
    std::cout << Model::DATA_COLLECTOR->current_utl_duration() << "\t";
}

void BurninFarmReporter::print_utl() {
    for (int i = 0; i < Model::DATA_COLLECTOR->UTL_duration().size(); i++) {
        std::cout << Model::DATA_COLLECTOR->UTL_duration()[i] << "\t";
    }

}

void BurninFarmReporter::print_LOI() {
    std::cout << Model::CONFIG->immune_system_information().duration_for_naive << "\t";
    std::cout << Model::CONFIG->immune_system_information().duration_for_fully_immune << "\t";
}

void BurninFarmReporter::print_moi_distribution() {
    for (int location = 0; location < Model::CONFIG->number_of_locations(); location++) {
        for (int i = 0; i < Model::DATA_COLLECTOR->multiple_of_infection_by_location()[location].size(); i++) {
            std::cout << Model::DATA_COLLECTOR->multiple_of_infection_by_location()[location][i] << "\t";
        }
    }

}

void BurninFarmReporter::print_mean_immune_by_location_age_class() {
    for (int location = 0; location < Model::CONFIG->number_of_locations(); location++) {
        for (int ac = 0; ac < Model::CONFIG->number_of_age_classes(); ac++) {
            std::cout << Model::DATA_COLLECTOR->total_immune_by_location_age_class()[location][ac] / Model::DATA_COLLECTOR->popsize_by_location_age_class()[location][ac] << "\t";
        }
    }
}
