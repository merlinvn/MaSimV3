/* 
 * File:   Config.cpp
 * Author: nguyentran
 * 
 * Created on March 27, 2013, 10:38 AM
 */

#include "Config.h"
#include <iostream>
#include <boost/foreach.hpp>
#include <gsl/gsl_cdf.h>
#include "HelperFunction.h"
#include "IStrategy.h"
//spatial
#include "SpatialMFTStrategy.h"
#include "SpatialCombinedMFTCyclingStrategy.h"
//end
#include "Model.h"
#include "Random.h"
#include "SCTherapy.h"
#include "StrategyBuilder.h"
#include "TherapyBuilder.h"
#include "NovelNonACTSwitchingStrategy.h"
#include "NestedSwitchingStrategy.h"
#include "CosineSeasonality.h"
#include "GeneralGravity.h"
#include "MMC_Zambia.h"
#include "Barabasi.h"
#include "ModelDataCollector.h"
#include <math.h>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <fstream>

//TODO: only use location 0 value when there is no other config information for other locations

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        boost::algorithm::trim(item);
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    return split(s, delim, elems);
}

Config::Config(Model *model) : model_(model), strategy_(NULL), strategy_db_(), therapy_db_(), drug_db_(NULL), genotype_db_(NULL), initial_parasite_info_(), tme_strategy_(NULL), location_db_(NULL) {
    total_time_ = -1;
    start_treatment_day_ = -1;
    start_collect_data_day_ = -1;

    p_treatment_ = -1;

    number_of_locations_ = -1;
    number_of_age_classes_ = -1;

    p_infection_from_an_infectious_bite_ = -1;

    birth_rate_ = -1;
    number_of_tracking_days_ = -1;

    modified_mutation_factor_ = -1;
    tf_window_size_ = -1;
    //    initial_parasite_info_.reserve(1);
    fraction_mosquitoes_interrupted_feeding_ = 0;

    modified_daily_cost_of_resistance_ = -1;
    modified_mutation_probability_ = -1;

}

Config::Config(const Config &orig) {
}

Config::~Config() {
    //    DeletePointer<Strategy>(strategy_);
    DeletePointer<IStrategy>(tme_strategy_);
    DeletePointer<DrugDatabase>(drug_db_);
    DeletePointer<IntGenotypeDatabase>(genotype_db_);

    BOOST_FOREACH(IStrategy* &i, strategy_db_) {
        delete i;
    }
    strategy_db_.clear();

    strategy_ = NULL;

    BOOST_FOREACH(Therapy* &i, therapy_db_) {
        delete i;
    }
    therapy_db_.clear();
}

void Config::read_from_file(const std::string &config_file_name) {

    YAML::Node config = YAML::LoadFile(config_file_name);
    total_time_ = config["total_time"].as<int>();
    start_treatment_day_ = config["start_treatment_day"].as<int>();
    start_collect_data_day_ = config["start_collect_data_day"].as<int>();

    p_treatment_ = config["p_treatment"].as<double>();

    //read population size
    population_size_by_location_.clear();
    beta_.clear();

    // load coordinate
    using_coordinate_ = config["load_coordinate"].as<int>();
    build_location_db(config);
   
    for (int i = 0; i < number_of_locations_; i++) {
        population_size_by_location_.push_back(location_db_->districts[i]->pop_size);
//        std::cout << "popsize:" << location_db_->districts[i]->pop_size << std::endl;
    }

    for (int i = 0; i < number_of_locations_; i++) {
        beta_.push_back(location_db_->districts[i]->beta);
//        std::cout << "beta:" << location_db_->districts[i]->beta << std::endl;
    }

    v_distance_by_location_.resize(number_of_locations_);
    for (int from_location = 0; from_location < number_of_locations_; from_location++) {
        v_distance_by_location_[from_location].resize(number_of_locations_);
        for (int to_location = 0; to_location < number_of_locations_; to_location++) {
            if (using_coordinate_ == 1) {
                v_distance_by_location_[from_location][to_location] = Vector2D::get_distance_in_Km_from_LatLon(location_db_->districts[from_location]->coordinate, location_db_->districts[to_location]->coordinate);
            } else {
                v_distance_by_location_[from_location][to_location] = Vector2D::get_Euclidean_distance(location_db_->districts[from_location]->coordinate, location_db_->districts[to_location]->coordinate);
            }

//            std::cout << "distance[" << from_location << "," << to_location << "]: " << v_distance_by_location_[from_location][to_location] << std::endl;
        }
    }

    //read seasonal beta information
    seasonal_beta_.a.clear();
    seasonal_beta_.phi_upper.clear();
    seasonal_beta_.phi_lower.clear();

    for (int loc = 0; loc < number_of_locations_; loc++) {
        seasonal_beta_.a.push_back(config["seasonal_beta"]["a"].as<double>());
        seasonal_beta_.phi_upper.push_back(config["seasonal_beta"]["phi_upper"].as<double>());
        seasonal_beta_.phi_lower.push_back(config["seasonal_beta"]["phi_lower"].as<double>());
    }

    number_of_age_classes_ = config["number_of_age_classes"].as<int>();

    //read_strategy_therapy_and_drug_information(config);

    p_infection_from_an_infectious_bite_ = config["p_infection_from_an_infectious_bite"].as<double>();

    age_structure_.clear();
    for (int i = 0; i < number_of_age_classes_; i++) {
        age_structure_.push_back(config["age_structure"][i].as<int>());
    }
    initial_age_structure_.clear();
    for (int i = 0; i < config["initial_age_structure"].size(); i++) {
        initial_age_structure_.push_back(config["initial_age_structure"][i].as<int>());
    }

    age_distribution_by_location_.clear();
    age_distribution_by_location_.assign(number_of_locations_, std::vector<double>());
    for (int loc = 0; loc < number_of_locations_; loc++) {
        for (int i = 0; i < config["age_distribution_by_location"][0].size(); i++) {
            //            age_distribution_by_location_[loc].push_back(config["age_distribution_by_location"][loc][i].as<double>());
            age_distribution_by_location_[loc].push_back(config["age_distribution_by_location"][0][i].as<double>());
        }
    }

    birth_rate_ = config["birth_rate"].as<double>();

    death_rate_by_age_.clear();
    for (int i = 0; i < number_of_age_classes_; i++) {
        death_rate_by_age_.push_back(config["death_rate_by_age"][i].as<double>());
    }

    number_of_tracking_days_ = config["number_of_tracking_days"].as<int>();

    mortality_when_treatment_fail_by_age_class_.clear();
    for (int i = 0; i < number_of_age_classes_; i++) {
        mortality_when_treatment_fail_by_age_class_.push_back(config["mortality_when_treatment_fail_by_age"][i].as<double>());
    }


    read_parasite_density_level(config["parasite_density_level"]);

    read_immune_system_information(config["immune_system_information"]);

    read_strategy_therapy_and_drug_information(config);

    read_relative_biting_rate_info(config);
    read_spatial_info(config);
    read_seasonal_info(config);
    read_spatial_external_population_info(config);

    read_initial_parasite_info(config);
    read_importation_parasite_info(config);
    read_importation_parasite_periodically_info(config);

    read_relative_infectivity_info(config);

    days_to_clinical_under_five_ = config["days_to_clinical_under_five"].as<int>();
    days_to_clinical_over_five_ = config["days_to_clinical_over_five"].as<int>();
    days_mature_gametocyte_under_five_ = config["days_mature_gametocyte_under_five"].as<int>();
    days_mature_gametocyte_over_five_ = config["days_mature_gametocyte_over_five"].as<int>();

    p_compliance_ = config["p_compliance"].as<double>();
    min_dosing_days_ = config["min_dosing_days"].as<int>();

    gametocyte_level_under_artemisinin_action_ = config["gametocyte_level_under_artemisinin_action"].as<double>();
    gametocyte_level_full_ = config["gametocyte_level_full"].as<double>();

    p_relapse_ = config["p_relapse"].as<double>();
    relapse_duration_ = config["relapse_duration"].as<int>();

    allow_new_coinfection_to_cause_symtoms_ = config["allow_new_coinfection_to_cause_symtoms"].as<bool>();
    update_frequency_ = config["update_frequency"].as<int>();
    report_frequency_ = config["report_frequency"].as<int>();

    TF_rate_ = config["TF_rate"].as<double>();

    tme_info_.tme_starting_day = config["tme_info"]["tme_starting_day"].as<int>();
    tme_info_.MDA_coverage.clear();
    tme_info_.MDA_duration.clear();
    for (int location = 0; location < number_of_locations_; location++) {
        //        tme_info_.MDA_coverage.push_back(config["tme_info"]["mda_coverage"][location].as<double>());
        //        tme_info_.MDA_duration.push_back(config["tme_info"]["mda_duration"][location].as<int>());
        tme_info_.MDA_coverage.push_back(config["tme_info"]["mda_coverage"][0].as<double>());
        tme_info_.MDA_duration.push_back(config["tme_info"]["mda_duration"][0].as<int>());
    }

    using_free_recombination_ = config["using_free_recombination"].as<bool>();
    tf_window_size_ = config["tf_window_size"].as<int>();

    using_age_dependent_bitting_level_ = config["using_age_dependent_bitting_level"].as<bool>();
    using_variable_probability_infectious_bites_cause_infection_ = config["using_variable_probability_infectious_bites_cause_infection"].as<bool>();
    fraction_mosquitoes_interrupted_feeding_ = config["fraction_mosquitoes_interrupted_feeding"].as<double>();
}

void Config::read_parasite_density_level(const YAML::Node &config) {

    log_parasite_density_level_.log_parasite_density_cured = config["log_parasite_density_cured"].as<double>();
    log_parasite_density_level_.log_parasite_density_from_liver = config["log_parasite_density_from_liver"].as<double>();
    log_parasite_density_level_.log_parasite_density_asymptomatic = config["log_parasite_density_asymptomatic"].as<double>();
    log_parasite_density_level_.log_parasite_density_clinical = config["log_parasite_density_clinical"].as<double>();
    log_parasite_density_level_.log_parasite_density_clinical_from = config["log_parasite_density_clinical_from"].as<double>();
    log_parasite_density_level_.log_parasite_density_clinical_to = config["log_parasite_density_clinical_to"].as<double>();
    log_parasite_density_level_.log_parasite_density_detectable = config["log_parasite_density_detectable"].as<double>();
    log_parasite_density_level_.log_parasite_density_pyrogenic = config["log_parasite_density_pyrogenic"].as<double>();
}

void Config::read_immune_system_information(const YAML::Node &config) {

    immune_system_information_.acquire_rate = config["b1"].as<double>();
    immune_system_information_.decay_rate = config["b2"].as<double>();

    immune_system_information_.duration_for_fully_immune = config["duration_for_fully_immune"].as<double>();
    immune_system_information_.duration_for_naive = config["duration_for_naive"].as<double>();

    double mean_initial_condition = config["mean_initial_condition"].as<double>();
    double sd_initial_condition = config["sd_initial_condition"].as<double>();

    if (sd_initial_condition == 0) {
        immune_system_information_.alpha_immune = mean_initial_condition;
        immune_system_information_.beta_immune = 0.0;
    } else {
        immune_system_information_.alpha_immune = mean_initial_condition * mean_initial_condition * (1 - mean_initial_condition) / (sd_initial_condition * sd_initial_condition) - mean_initial_condition;
        immune_system_information_.beta_immune = immune_system_information_.alpha_immune / mean_initial_condition - immune_system_information_.alpha_immune;
    }

    immune_system_information_.immune_inflation_rate = config["immune_inflation_rate"].as<double>();

    immune_system_information_.min_clinical_probability = config["min_clinical_probability"].as<double>();
    immune_system_information_.max_clinical_probability = config["max_clinical_probability"].as<double>();

    immune_system_information_.immune_effect_on_progression_to_clinical = config["immune_effect_on_progression_to_clinical"].as<double>();

    immune_system_information_.c_min = pow(10, -(log_parasite_density_level_.log_parasite_density_asymptomatic - log_parasite_density_level_.log_parasite_density_cured) / immune_system_information_.duration_for_fully_immune);
    immune_system_information_.c_max = pow(10, -(log_parasite_density_level_.log_parasite_density_asymptomatic - log_parasite_density_level_.log_parasite_density_cured) / immune_system_information_.duration_for_naive);
    //    std::cout << immune_system_information_.c_min << std::endl;
    //    std::cout << immune_system_information_.c_max << std::endl;

    immune_system_information_.age_mature_immunity = config["age_mature_immunity"].as<double>();
    immune_system_information_.factor_effect_age_mature_immunity = config["factor_effect_age_mature_immunity"].as<double>();

    // implement inlation rate
    double acR = immune_system_information_.acquire_rate;
    immune_system_information_.acquire_rate_by_age.clear();
    for (int age = 0; age <= 80; age++) {
        double factor = 1;
        if (age < immune_system_information().age_mature_immunity) {
            factor = (age == 0) ? 0.5 : age;

            factor = factor / immune_system_information().age_mature_immunity;
            factor = pow(factor, immune_system_information().factor_effect_age_mature_immunity);
        }

        immune_system_information_.acquire_rate_by_age.push_back(factor * acR);

        acR *= (1 + immune_system_information_.immune_inflation_rate);
        //        std::cout << acR << std::endl;
    }
    assert(immune_system_information_.acquire_rate_by_age.size() == 81);
}

void Config::read_strategy_therapy_and_drug_information(const YAML::Node &config) {
    read_genotype_info(config);
    build_drug_and_parasite_db(config);

    // read tf_testing_day
    tf_testing_day_ = config["tf_testing_day"].as<int>();

    //    read_all_therapy
    for (int i = 0; i < config["TherapyInfo"].size(); i++) {
        Therapy* t = read_therapy(config["TherapyInfo"], i);
        therapy_db_.push_back(t);
    }

    for (int i = 0; i < config["StrategyInfo"].size(); i++) {
        IStrategy* s = read_strategy(config["StrategyInfo"], i);
        strategy_db_.push_back(s);
    }

    strategy_ = strategy_db_[config["main_strategy_id"].as<int>()];
    
    if (strategy_->get_type() == IStrategy::NestedSwitching) {
        ((NestedSwitchingStrategy*) strategy_)->initialize_update_time();
    }
}

IStrategy* Config::read_strategy(const YAML::Node& n, const int& strategy_id) {
    std::string s_id = NumberToString<int>(strategy_id);

    IStrategy* result = StrategyBuilder::build(n[s_id], strategy_id);

    //    std::cout << result->to_string() << std::endl;
    return result;
}

Therapy* Config::read_therapy(const YAML::Node& n, const int& therapy_id) {
    std::string t_id = NumberToString<int>(therapy_id);
    Therapy* t = TherapyBuilder::build(n[t_id], therapy_id);

    return t;
}

void Config::build_drug_and_parasite_db(const YAML::Node &config) {
    // build parasite db
    build_parasite_db();

    //build drug DB
    build_drug_db(config);

    //read fake_efficacy_table
    fake_efficacy_table_.clear();
    fake_efficacy_table_.assign(genotype_db_->db().size(), std::vector<double>());

    for (int g_id = 0; g_id < genotype_db_->db().size(); g_id++) {
        for (int i = 0; i < config["fake_efficacy_table"][g_id].size(); i++) {
            fake_efficacy_table_[g_id].push_back(config["fake_efficacy_table"][g_id][i].as<double>());
        }
    }
}

void Config::read_genotype_info(const YAML::Node &config) {
    genotype_info_.loci_vector.clear();
    for (int i = 0; i < config["genotype_info"]["loci"].size(); i++) {
        Locus l;
        l.position = config["genotype_info"]["loci"][i]["position"].as<int>();

        for (int j = 0; j < config["genotype_info"]["loci"][i]["alleles"].size(); j++) {
            Allele al;
            al.value = config["genotype_info"]["loci"][i]["alleles"][j]["value"].as<int>();
            al.name = config["genotype_info"]["loci"][i]["alleles"][j]["allele_name"].as<std::string>();
            al.short_name = config["genotype_info"]["loci"][i]["alleles"][j]["short_name"].as<std::string>();
            al.mutation_level = config["genotype_info"]["loci"][i]["alleles"][j]["mutation_level"].as<int>();
            al.daily_cost_of_resistance = config["genotype_info"]["loci"][i]["alleles"][j]["daily_cost_of_resistance"].as<double>();
            for (int c = 0; c < config["genotype_info"]["loci"][i]["alleles"][j]["can_mutate_to"].size(); c++) {
                //                al.mutation_value_up.push_back(config["genotype_info"]["loci"][i]["alleles"][j]["mutation_up"][c].as<int>());
                al.mutation_values.push_back(config["genotype_info"]["loci"][i]["alleles"][j]["can_mutate_to"][c].as<int>());
            }

            l.alleles.push_back(al);
        }

        genotype_info_.loci_vector.push_back(l);
    }
    //
    //    for (int c = 0; c < genotype_info_.loci_vector[2].alleles[2].mutation_values.size(); c++) {
    //        std::cout << genotype_info_.loci_vector[2].alleles[2].mutation_values[c] << std::endl;
    //
    //    }
}

void Config::build_drug_db(const YAML::Node &config) {
    DeletePointer<DrugDatabase>(drug_db_);
    drug_db_ = new DrugDatabase();

    for (int i = 0; i < config["drugInfo"].size(); i++) {
        DrugType *dt = read_drugtype(config, i);
        //        std::cout << i << std::endl;
        drug_db_->add(dt);
    }

    //get EC50 table and compute EC50^n
    EC50_power_n_table_.clear();
    EC50_power_n_table_.assign(genotype_db_->db().size(), std::vector<double>());

    for (int g_id = 0; g_id < genotype_db_->db().size(); g_id++) {
        for (int i = 0; i < drug_db_->drug_db().size(); i++) {
            EC50_power_n_table_[g_id].push_back(drug_db_->drug_db()[i]->inferEC50(genotype_db_->db()[g_id]));
        }
    }
    //    std::cout << "ok " << std::endl;

    for (int g_id = 0; g_id < genotype_db_->db().size(); g_id++) {
        for (int i = 0; i < drug_db_->drug_db().size(); i++) {
            EC50_power_n_table_[g_id][i] = pow(EC50_power_n_table_[g_id][i], drug_db_->get(i)->n());
        }
    }
}

void Config::build_parasite_db() {

    DeletePointer<IntGenotypeDatabase>(genotype_db_);
    genotype_db_ = new IntGenotypeDatabase();

    int number_of_genotypes = 1;
    for (int i = 0; i < genotype_info_.loci_vector.size(); i++) {
        number_of_genotypes *= genotype_info_.loci_vector[i].alleles.size();
    }

    for (int i = 0; i < number_of_genotypes; i++) {
        IntGenotype* int_genotype = new IntGenotype(i);
        //        std::cout << *int_genotype << std::endl;
        genotype_db_->add(int_genotype);
    }

    genotype_db_->initialize_matting_matrix();
    number_of_parasite_types_ = genotype_db_->db().size();
}

SeasonalStructure *Config::read_seasonal_structure(const YAML::Node &config, const YAML::Node &n, const std::string &structure_name) {
    SeasonalStructure *s;
    if (structure_name == "CosineSeasonality") {
        s = new CosineSeasonality();
        ((CosineSeasonality *) s)->set_test(n[structure_name]["test"].as<int>());
        ((CosineSeasonality *) s)->set_peak(n[structure_name]["peak"].as<double>());
        ((CosineSeasonality *) s)->set_trough(n[structure_name]["trough"].as<double>());
        ((CosineSeasonality *) s)->set_phi_upper(n[structure_name]["phi_upper"].as<int>());
        ((CosineSeasonality *) s)->set_phi_lower(n[structure_name]["phi_lower"].as<int>());
    }
    return s;
}

SpatialStructure *Config::read_spatial_structure(const YAML::Node &config, const YAML::Node &n, const std::string &structure_name) {
    SpatialStructure *s;
    if (structure_name == "GeneralGravity") {
        s = new GeneralGravity();
        ((GeneralGravity *) s)->set_test(n[structure_name]["test"].as<int>());
    } else if (structure_name == "MMC_Zambia") {
        s = new MMC_Zambia();
        ((MMC_Zambia *) s)->set_test(n[structure_name]["test"].as<int>());
        ((MMC_Zambia *) s)->set_theta(n[structure_name]["theta"].as<double>());
        ((MMC_Zambia *) s)->set_alpha(n[structure_name]["alpha"].as<double>());
        ((MMC_Zambia *) s)->set_beta(n[structure_name]["beta"].as<double>());
        ((MMC_Zambia *) s)->set_gamma(n[structure_name]["gamma"].as<double>());
    } else if (structure_name == "Barabasi") {
        s = new Barabasi();
        ((Barabasi *) s)->set_test(n[structure_name]["test"].as<int>());
        ((Barabasi *) s)->set_r_g_0(n[structure_name]["r_g_0"].as<double>());
        ((Barabasi *) s)->set_beta_r(n[structure_name]["beta_r"].as<double>());
        ((Barabasi *) s)->set_kappa(n[structure_name]["kappa"].as<double>());
    }
    return s;
}


void Config::build_location_db(const YAML::Node &config) {
    location_db_ = new LocationInfo();
    if (using_coordinate_ == 1) {
        read_location_db_using_coordinate_info(config["Cambodia_info"]);
        number_of_locations_ = location_db_->districts.size();
    } else {
        read_location_db_using_normal_info(config);
    }
}

void Config::read_location_db_using_normal_info(const YAML::Node &config) {

    number_of_locations_ = config["number_of_locations"].as<int>();
    number_of_column_in_grid_ = config["number_of_column_in_grid"].as<int>();
    grid_unit_in_km_ = config["grid_unit_in_km"].as<double>();

    //read popsize & beta
    for (int i = 0; i < number_of_locations_; i++) {

        District *d = new District();
        d->pop_size = config["population_size_by_location"][i].as<int>();
        d->beta = config["beta"][i].as<double>();

        d->coordinate.x = (float) (i / number_of_column_in_grid_);
        d->coordinate.y = (float) (i % number_of_column_in_grid_);

        location_db_->districts.push_back(d);
    }

}

void Config::read_location_db_using_coordinate_info(const YAML::Node &config) {
    //loop each province
    int district_id = 0;

    for (int i = 0; i < config.size(); i++) {
        Province *p = new Province();
        p->id = i;
        p->pop_size = config[i]["pop_size"].as<int>();
        location_db_->provinces.push_back(p);


        // double province_beta = n["beta"].as<double>();
        //loop each district

        for (int j = 0; j < config[i]["districts_info"]["pop_proportion_by_districts"].size(); j++) {
            District *d = new District();
            d->id = district_id;
            district_id++;
            d->province_id = p->id;
            d->pop_size = p->pop_size * config[i]["districts_info"]["pop_proportion_by_districts"][j].as<double>();
            d->beta = config[i]["districts_info"]["beta_by_district"][j].as<double>();
            d->coordinate.x = config[i]["districts_info"]["coordinates_by_district"][j][0].as<double>();
            d->coordinate.y = config[i]["districts_info"]["coordinates_by_district"][j][1].as<double>();

            p->districts.push_back(d);
            location_db_->districts.push_back(d);
        }
    }
    
DrugType * Config::read_drugtype(const YAML::Node& config, const int& drug_id) {
    DrugType* dt = new DrugType();
    dt->set_id(drug_id);

    std::string drug_id_s = NumberToString<int>(drug_id);
    const YAML::Node& n = config["drugInfo"][drug_id_s];

    dt->set_drug_half_life(n["half_life"].as<double>());
    dt->set_maximum_parasite_killing_rate(n["maximum_parasite_killing_rate"].as<double>());
    dt->set_n(n["n"].as<double>());
    //    dt->set_EC50(n["EC50"].as<double>());

    //    std::cout <<dt->drug_half_life() << "-" << dt->maximum_parasite_killing_rate() << "-" << dt->n() << "-" << dt->EC50() << std::endl;
    for (int i = 0; i < n["age_specific_drug_concentration_sd"].size(); i++) {
        dt->age_group_specific_drug_concentration_sd().push_back(n["age_specific_drug_concentration_sd"][i].as<double>());
    }
    //    assert(dt->age_group_specific_drug_concentration_sd().size() == 15);

    dt->set_p_mutation(n["mutation_probability"].as<double>());

    dt->affecting_loci().clear();
    for (int i = 0; i < n["affecting_loci"].size(); i++) {
        dt->affecting_loci().push_back(n["affecting_loci"][i].as<int>());
    }

    dt->selecting_alleles().clear();
    dt->selecting_alleles().assign(n["affecting_loci"].size(), IntVector());
    for (int i = 0; i < n["affecting_loci"].size(); i++) {
        for (int j = 0; j < n["selecting_alleles"][i].size(); j++) {
            dt->selecting_alleles()[i].push_back(n["selecting_alleles"][i][j].as<int>());
        }
    }

    dt->set_ec50map(n["EC50"].as<std::map < std::string, double> >());

    //    auto ec50Node = n["EC50"];
    //    for (YAML::const_iterator it = ec50Node.begin(); it != ec50Node.end(); it++) {
    //        std::string key = it->first.as<std::string>();
    //        double value = it->second.as<double>();
    //        std::cout << key << ":::::" << value << std::endl;
    //    }

    dt->set_k(n["k"].as<double>());

    if (drug_id == config["artemisinin_drug_id"].as<int>()) {
        dt->set_drug_family(DrugType::Artemisinin);
    } else if (drug_id == config["lumefantrine_drug_id"].as<int>()) {
        dt->set_drug_family(DrugType::Lumefantrine);
    } else {
        dt->set_drug_family(DrugType::Other);
    }
    return dt;
}

void Config::read_relative_biting_rate_info(const YAML::Node &config) {
    const YAML::Node &n = config["relative_bitting_info"];

    relative_bitting_information_.max_relative_biting_value = n["max_relative_biting_value"].as<double>();

    relative_bitting_information_.number_of_biting_levels = n["number_of_biting_levels"].as<double>();

    relative_bitting_information_.scale = n["biting_level_distribution"]["Exponential"]["scale"].as<double>();

    relative_bitting_information_.mean = n["biting_level_distribution"]["Gamma"]["mean"].as<double>();
    relative_bitting_information_.sd = n["biting_level_distribution"]["Gamma"]["sd"].as<double>();

    //calculate density and level value here
    calculate_relative_biting_density();
}

void Config::calculate_relative_biting_density() {
    double var = relative_bitting_information_.sd * relative_bitting_information_.sd;
    double b = var / (relative_bitting_information_.mean - 1); //theta
    double a = (relative_bitting_information_.mean - 1) / b; //k

    relative_bitting_information_.v_biting_level_density.clear();
    relative_bitting_information_.v_biting_level_value.clear();

    double max = relative_bitting_information_.max_relative_biting_value - 1; //maxRelativeBiting -1
    int numberOfLevel = relative_bitting_information_.number_of_biting_levels;

    double step = max / (double) (numberOfLevel - 1);

    int j = 0;
    double oldP = 0;
    double sum = 0;

    for (double i = 0; i <= max + 0.0001; i += step) {
        double p = gsl_cdf_gamma_P(i + step, a, b);
        double value = 0;
        value = (j == 0) ? p : p - oldP;
        relative_bitting_information_.v_biting_level_density.push_back(value);
        oldP = p;
        relative_bitting_information_.v_biting_level_value.push_back(i + 1);
        sum += value;
        j++;
    }

    //normalized
    double t = 0;
    for (int i = 0; i < relative_bitting_information_.v_biting_level_density.size(); i++) {
        relative_bitting_information_.v_biting_level_density[i] = relative_bitting_information_.v_biting_level_density[i] + (1 - sum) / relative_bitting_information_.v_biting_level_density.size();
        t += relative_bitting_information_.v_biting_level_density[i];
    }

    assert(relative_bitting_information_.number_of_biting_levels == relative_bitting_information_.v_biting_level_density.size());
    assert(relative_bitting_information_.number_of_biting_levels == relative_bitting_information_.v_biting_level_value.size());
    assert(fabs(t - 1) < 0.0001);
    if (Model::RANDOM != NULL) {
        Model::RANDOM->bitting_level_generator().set_level_density(&relative_bitting_information_.v_biting_level_density);
    }

    //    for (int i = 0; i < relative_bitting_information_.v_biting_level_density.size(); i++) {
    //        std::cout << i << "\t" << relative_bitting_information_.v_biting_level_value[i] << "\t"
    //                << relative_bitting_information_.v_biting_level_density[i] << std::endl;
    //    }
}

void Config::read_spatial_info(const YAML::Node &config) {
    const YAML::Node &n = config["spatial_information"];

    spatial_information_.max_relative_moving_value = n["max_relative_moving_value"].as<double>();

    spatial_information_.number_of_moving_levels = n["number_of_moving_levels"].as<double>();

    spatial_information_.scale = n["moving_level_distribution"]["Exponential"]["scale"].as<double>();

    spatial_information_.mean = n["moving_level_distribution"]["Gamma"]["mean"].as<double>();
    spatial_information_.sd = n["moving_level_distribution"]["Gamma"]["sd"].as<double>();

    //calculate density and level value here

    double var = spatial_information_.sd * spatial_information_.sd;

    double b = var / (spatial_information_.mean - 1); //theta
    double a = (spatial_information_.mean - 1) / b; //k

    spatial_information_.v_moving_level_density.clear();
    spatial_information_.v_moving_level_value.clear();

    double max = spatial_information_.max_relative_moving_value - 1; //maxRelativeBiting -1
    int numberOfLevel = spatial_information_.number_of_moving_levels;

    double step = max / (double) (numberOfLevel - 1);

    int j = 0;
    double oldP = 0;
    double sum = 0;

    for (double i = 0; i <= max + 0.0001; i += step) {
        double p = gsl_cdf_gamma_P(i + step, a, b);
        double value = 0;
        value = (j == 0) ? p : p - oldP;
        spatial_information_.v_moving_level_density.push_back(value);
        oldP = p;
        spatial_information_.v_moving_level_value.push_back(i + 1);
        sum += value;
        j++;
    }

    //normalized
    double t = 0;
    for (int i = 0; i < spatial_information_.v_moving_level_density.size(); i++) {
        spatial_information_.v_moving_level_density[i] = spatial_information_.v_moving_level_density[i] + (1 - sum) / spatial_information_.v_moving_level_density.size();
        t += spatial_information_.v_moving_level_density[i];
    }

    assert(spatial_information_.number_of_moving_levels == spatial_information_.v_moving_level_density.size());
    assert(spatial_information_.number_of_moving_levels == spatial_information_.v_moving_level_value.size());
    assert(fabs(t - 1) < 0.0001);

    spatial_information_.circulation_percent = n["circulation_percent"].as<double>();

    double length_of_stay_mean = n["length_of_stay"]["mean"].as<double>();
    double length_of_stay_sd = n["length_of_stay"]["sd"].as<double>();

    double stay_variance = length_of_stay_sd * length_of_stay_sd;
    double k = stay_variance / length_of_stay_mean; //k = 20
    double theta = length_of_stay_mean / k; //theta = 0.25

    spatial_information_.length_of_stay_theta = theta;
    spatial_information_.length_of_stay_k = k;

    if (Model::RANDOM != NULL) {
        Model::RANDOM->moving_level_generator().set_level_density(&spatial_information_.v_moving_level_density);
    }

    spatial_structure_ = read_spatial_structure(config, config["spatial_information"], "GeneralGravity");
    spatial_structure_db_.insert(std::pair<int, SpatialStructure *>(spatial_structure_->to_int(), spatial_structure_));
    spatial_structure_ = read_spatial_structure(config, config["spatial_information"], "MMC_Zambia");
    spatial_structure_db_.insert(std::pair<int, SpatialStructure *>(spatial_structure_->to_int(), spatial_structure_));
    spatial_structure_ = read_spatial_structure(config, config["spatial_information"], "Barabasi");
    spatial_structure_db_.insert(std::pair<int, SpatialStructure *>(spatial_structure_->to_int(), spatial_structure_));

    std::string spatial_structure_name = config["spatial_information"]["type_of_movement"].as<std::string>();

    BOOST_FOREACH(SpatialStructurePtrMap::value_type &i, spatial_structure_db_) {
        if (i.second->to_string() == spatial_structure_name) {
            spatial_structure_ = i.second;
        }
    }
}

void Config::read_seasonal_info(const YAML::Node &config) {
    seasonal_structure_ = read_seasonal_structure(config, config["seasonality"], "CosineSeasonality");
    seasonal_structure_db_.insert(std::pair<int, SeasonalStructure *>(seasonal_structure_->to_int(), seasonal_structure_));

    std::string seasonal_structure_name = config["seasonality"]["seasonality_function"].as<std::string>();

    BOOST_FOREACH(SeasonalStructurePtrMap::value_type &i, seasonal_structure_db_) {
        if (i.second->to_string() == seasonal_structure_name) {
            seasonal_structure_ = i.second;
        }
    }
}

void Config::read_spatial_external_population_info(const YAML::Node &config) {
    const YAML::Node &n = config["spatial_external_population_information"];

    spatial_external_population_information_.max_relative_moving_value = n["max_relative_moving_value"].as<double>();

    spatial_external_population_information_.number_of_moving_levels = n["number_of_moving_levels"].as<double>();

    spatial_external_population_information_.scale = n["moving_level_distribution"]["Exponential"]["scale"].as<double>();

    spatial_external_population_information_.mean = n["moving_level_distribution"]["Gamma"]["mean"].as<double>();
    spatial_external_population_information_.sd = n["moving_level_distribution"]["Gamma"]["sd"].as<double>();

    //calculate density and level value here

    double var = spatial_external_population_information_.sd * spatial_external_population_information_.sd;

    double b = var / (spatial_external_population_information_.mean - 1); //theta
    double a = (spatial_external_population_information_.mean - 1) / b; //k

    spatial_external_population_information_.v_moving_level_density.clear();
    spatial_external_population_information_.v_moving_level_value.clear();

    double max = spatial_external_population_information_.max_relative_moving_value - 1; //maxRelativeBiting -1
    int numberOfLevel = spatial_external_population_information_.number_of_moving_levels;

    double step = max / (double) (numberOfLevel - 1);

    int j = 0;
    double oldP = 0;
    double sum = 0;

    for (double i = 0; i <= max + 0.0001; i += step) {
        double p = gsl_cdf_gamma_P(i + step, a, b);
        double value = 0;
        value = (j == 0) ? p : p - oldP;
        spatial_external_population_information_.v_moving_level_density.push_back(value);
        oldP = p;
        spatial_external_population_information_.v_moving_level_value.push_back(i + 1);
        sum += value;
        j++;
    }

    //normalized
    double t = 0;
    for (int i = 0; i < spatial_external_population_information_.v_moving_level_density.size(); i++) {
        spatial_external_population_information_.v_moving_level_density[i] = spatial_external_population_information_.v_moving_level_density[i] + (1 - sum) / spatial_information_.v_moving_level_density.size();
        t += spatial_external_population_information_.v_moving_level_density[i];
    }

    assert(spatial_external_population_information_.number_of_moving_levels == spatial_external_population_information_.v_moving_level_density.size());
    assert(spatial_external_population_information_.number_of_moving_levels == spatial_external_population_information_.v_moving_level_value.size());
    assert(fabs(t - 1) < 0.0001);

    for (int i = 0; i < number_of_locations_; i++) {
        //        spatial_external_population_information_.circulation_percent.push_back(n["circulation_percent"][i].as<double>());
        spatial_external_population_information_.circulation_percent.push_back(n["circulation_percent"][0].as<double>());
    }
    //    spatial_external_population_information_.circulation_percent = n["circulation_percent"].as<double>();

    double length_of_stay_mean = n["length_of_stay"]["mean"].as<double>();
    double length_of_stay_sd = n["length_of_stay"]["sd"].as<double>();

    double stay_variance = length_of_stay_sd * length_of_stay_sd;
    double k = stay_variance / length_of_stay_mean; //k
    double theta = length_of_stay_mean / k; //theta

    spatial_external_population_information_.length_of_stay_theta = theta;
    spatial_external_population_information_.length_of_stay_k = k;

    if (Model::RANDOM != NULL) {
        Model::RANDOM->external_population_moving_level_generator().set_level_density(&spatial_external_population_information_.v_moving_level_density);
    }

    for (int i = 0; i < number_of_locations_; i++) {
        //        spatial_external_population_information_.daily_EIR.push_back(n["daily_EIR"][i].as<double>());
        //        spatial_external_population_information_.seasonal_EIR.a.push_back(n["seasonal_EIR"]["a"][i].as<double>());
        //        spatial_external_population_information_.seasonal_EIR.phi.push_back(n["seasonal_EIR"]["phi"][i].as<double>());
        spatial_external_population_information_.daily_EIR.push_back(n["daily_EIR"][0].as<double>());
        spatial_external_population_information_.seasonal_EIR.a.push_back(n["seasonal_EIR"]["a"].as<double>());
        spatial_external_population_information_.seasonal_EIR.phi_upper.push_back(n["seasonal_EIR"]["phi_upper"].as<double>());
        spatial_external_population_information_.seasonal_EIR.phi_lower.push_back(n["seasonal_EIR"]["phi_lower"].as<double>());
    }
    //    spatial_external_population_information_.daily_EIR = n["daily_EIR"].as<double>();
}

void Config::read_initial_parasite_info(const YAML::Node &config) {
    const YAML::Node &n = config["initial_parasite_info"];

    //    for (int i = 0; i < n.size(); i++) {
    //        int location = n[i]["location_id"].as<int>();
    //        if (location < number_of_locations_) {
    //            for (int j = 0; j < n[i]["parasite_info"].size(); j++) {
    //                //            InitialParasiteInfo ipi;
    //                //            ipi.location = location;
    //                int parasite_type_id = n[i]["parasite_info"][j]["parasite_type_id"].as<int>();
    //                double prevalence = n[i]["parasite_info"][j]["prevalence"].as<double>();
    //                initial_parasite_info_.push_back(InitialParasiteInfo(location, parasite_type_id, prevalence));
    //            }
    //        }
    //    }

    for (int loc = 0; loc < number_of_locations_; loc++) {
        for (int j = 0; j < n[0]["parasite_info"].size(); j++) {
            int parasite_type_id = n[0]["parasite_info"][j]["parasite_type_id"].as<int>();
            double prevalence = n[0]["parasite_info"][j]["prevalence"].as<double>();
            initial_parasite_info_.push_back(InitialParasiteInfo(loc, parasite_type_id, prevalence));
        }
    }
}

void Config::read_importation_parasite_info(const YAML::Node &config) {
    const YAML::Node &n = config["introduce_parasite"];

    for (int i = 0; i < n.size(); i++) {
        int location = n[i]["location"].as<int>();
        if (location < number_of_locations_) {
            for (int j = 0; j < n[i]["parasite_info"].size(); j++) {
                //            InitialParasiteInfo ipi;
                //            ipi.location = location;
                int parasite_type_id = n[i]["parasite_info"][j]["genotype_id"].as<int>();
                int time = n[i]["parasite_info"][j]["time"].as<int>();
                int num = n[i]["parasite_info"][j]["number_of_cases"].as<int>();
                importation_parasite_info_.push_back(ImportationParasiteInfo(location, parasite_type_id, time, num));
            }
        }
    }
}

void Config::read_importation_parasite_periodically_info(const YAML::Node &config) {
    const YAML::Node &n = config["introduce_parasite_periodically"];
    for (int i = 0; i < n.size(); i++) {
        int location = n[i]["location"].as<int>();
        if (location < number_of_locations_) {
            for (int j = 0; j < n[i]["parasite_info"].size(); j++) {
                //            InitialParasiteInfo ipi;
                //            ipi.location = location;
                int parasite_type_id = n[0]["parasite_info"][j]["genotype_id"].as<int>();
                int dur = n[0]["parasite_info"][j]["duration"].as<int>();
                int num = n[0]["parasite_info"][j]["number_of_cases"].as<int>();
                int start_day = n[0]["parasite_info"][j]["start_day"].as<int>();
                importation_parasite_periodically_info_.push_back(ImportationParasitePeriodicallyInfo(location, parasite_type_id, dur, num, start_day));
            }
        }
    }
}

void Config::read_relative_infectivity_info(const YAML::Node &config) {
    const YAML::Node &n = config["relative_infectivity"];

    relative_infectivity_.sigma = n["sigma"].as<double>();
    double ro = n["ro"].as<double>();
    double blood_meal_volume = n["blood_meal_volume"].as<double>();

    double d_star = 1 / blood_meal_volume;

    relative_infectivity_.ro_star = (log(ro) - log(d_star)) / relative_infectivity_.sigma;

    relative_infectivity_.sigma = log(10) / relative_infectivity_.sigma;
    //    std::cout << relative_infectivity_.sigma << "\t" << relative_infectivity_.ro_star<< std::endl;
}

void Config::override_parameters(const std::string &override_file, const int &pos) {
    if (pos == -1)
        return;
    std::ifstream ifs(override_file.c_str());
    if (!ifs && override_file != "") {
        std::cout << "Error" << std::endl;
        exit(1);
    }

    std::string buf;
    getline(ifs, buf);

    //header of file containt all overrides parameters name
    std::vector<std::string> override_header = split(buf, '\t');

    //goto the pos line in the file
    for (int i = 0; (i < pos) && getline(ifs, buf); i++) {
    }
    //buff contain the parameters
    std::vector<std::string> override_parameters = split(buf, '\t');

    //override parameter
    for (int i = 0; i < override_parameters.size(); i++) {
        if (override_parameters[i] != "nil") {
            override_1_parameter(override_header[i], override_parameters[i]);
        }
    }
    //re-calculate dependent variables
    //    CalculateDependentVariables();
}

void Config::override_1_parameter(const std::string &parameter_name, const std::string &parameter_value) {
    if (parameter_name == "population_size") {
        population_size_by_location_[0] = atoi(parameter_value.c_str());
    }

    if (parameter_name == "total_time") {
        total_time_ = atoi(parameter_value.c_str());
    }

    if (parameter_name == "beta") {
        beta_[0] = atof(parameter_value.c_str());
    }

    if (parameter_name == "f") {
        p_treatment_ = atof(parameter_value.c_str());
    }

    if (parameter_name == "daily_cost_of_resistance_factor") {
        modified_daily_cost_of_resistance_ = atof(parameter_value.c_str());

        for (int i = 0; i < genotype_info_.loci_vector.size(); i++) {
            for (int j = 0; j < genotype_info_.loci_vector[i].alleles.size(); j++) {
                genotype_info_.loci_vector[i].alleles[j].daily_cost_of_resistance *= modified_daily_cost_of_resistance_;
            }
        }
        build_parasite_db();
    }

    if (parameter_name == "daily_cost_of_resistance_copies_pfmdr") {
        double dcr = atof(parameter_value.c_str());
        // modify allele 4 5 6 7 of the mdr locus
        genotype_info_.loci_vector[1].alleles[4].daily_cost_of_resistance = dcr;
        genotype_info_.loci_vector[1].alleles[5].daily_cost_of_resistance = 1 - (1 - genotype_info_.loci_vector[1].alleles[1].daily_cost_of_resistance) * (1 - dcr);
        genotype_info_.loci_vector[1].alleles[6].daily_cost_of_resistance = 1 - (1 - genotype_info_.loci_vector[1].alleles[2].daily_cost_of_resistance) * (1 - dcr);
        genotype_info_.loci_vector[1].alleles[7].daily_cost_of_resistance = 1 - (1 - genotype_info_.loci_vector[1].alleles[3].daily_cost_of_resistance) * (1 - dcr);
        build_parasite_db();
    }

    if (parameter_name == "z") {
        immune_system_information_.immune_effect_on_progression_to_clinical = atof(parameter_value.c_str());
    }

    if (parameter_name == "kappa") {
        immune_system_information_.factor_effect_age_mature_immunity = atof(parameter_value.c_str());

        // implement inlation rate
        double acR = immune_system_information_.acquire_rate;
        immune_system_information_.acquire_rate_by_age.clear();
        for (int age = 0; age <= 80; age++) {
            double factor = 1;
            if (age < immune_system_information().age_mature_immunity) {
                factor = (age == 0) ? 0.5 : age;

                factor = factor / immune_system_information().age_mature_immunity;
                factor = pow(factor, immune_system_information().factor_effect_age_mature_immunity);
            }

            immune_system_information_.acquire_rate_by_age.push_back(factor * acR);

            acR *= (1 + immune_system_information_.immune_inflation_rate);
        }

        assert(immune_system_information_.acquire_rate_by_age.size() == 81);
    }

    if (parameter_name == "relative_bitting_sd") {
        relative_bitting_information_.sd = atof(parameter_value.c_str());
        calculate_relative_biting_density();
    }

    if (parameter_name == "k") {
        modified_mutation_factor_ = atof(parameter_value.c_str());
        for (DrugTypePtrMap::iterator it = drug_db_->drug_db().begin(); it != drug_db_->drug_db().end(); it++) {
            it->second->set_k(modified_mutation_factor_);
        }
    }

    if (parameter_name == "drug_half_life") {
        modified_drug_half_life_ = atof(parameter_value.c_str());
    }

    if (parameter_name == "strategy") {
        //override with the id from the override.txt
        int strategy_id = atoi(parameter_value.c_str());
        strategy_ = strategy_db_[strategy_id];
        if (strategy_->get_type() == IStrategy::NestedSwitching) {
            ((NestedSwitchingStrategy*) strategy_)->initialize_update_time();
        }
    }

    if (parameter_name == "dosing_days") {
        double dosing_days = atof(parameter_value.c_str());
        for (TherapyPtrVector::iterator it = therapy_db_.begin(); it != therapy_db_.end(); it++) {

            SCTherapy* scTherapy = dynamic_cast<SCTherapy*> (*it);
            if (scTherapy != NULL) {
                scTherapy->set_dosing_day(dosing_days);
            }
        }
    }

    if (parameter_name == "mutation_probability") {
        modified_mutation_probability_ = atof(parameter_value.c_str());
        for (DrugTypePtrMap::iterator it = drug_db_->drug_db().begin(); it != drug_db_->drug_db().end(); it++) {
            it->second->set_p_mutation(modified_mutation_probability_);

        }
        //        std::cout<< Model::CONFIG->drug_db()->drug_db().begin()->second->p_mutation() << std::endl;
    }

    if (parameter_name == "mutation_factor") {
        double mutation_factor = atof(parameter_value.c_str());
        for (DrugTypePtrMap::iterator it = drug_db_->drug_db().begin(); it != drug_db_->drug_db().end(); it++) {
            it->second->set_p_mutation(it->second->p_mutation() * mutation_factor);
        }
    }

    if (parameter_name == "initial_genotype") {
        int genotypeId = atoi(parameter_value.c_str());
        initial_parasite_info_[0].parasite_type_id = genotypeId;
        importation_parasite_periodically_info_[0].parasite_type_id = genotypeId;
    }

    if (parameter_name == "importation_period") {
        int importation_period = atoi(parameter_value.c_str());
        for (int i = 0; i < importation_parasite_periodically_info_.size(); i++) {
            if (importation_parasite_periodically_info_[i].parasite_type_id == -1) {
                importation_parasite_periodically_info_[i].duration = importation_period;
            }
        }
    }
    if (parameter_name == "fraction_non_art_replacement") {
        double fnar = atof(parameter_value.c_str());
        NovelNonACTSwitchingStrategy* s = dynamic_cast<NovelNonACTSwitchingStrategy*> (Model::CONFIG->strategy());
        if (s != NULL) {
            s->set_fraction_non_art_replacement(fnar);
        }
    }

    // TACT id and switching day will be modify by create new strategy in the .yml input file 

        spatial_information_.circulation_percent = atof(parameter_value.c_str());
    }

    if (parameter_name == "mean_beta_lognormal") {
        mean_beta_lognormal_ = atof(parameter_value.c_str());
    }
}

void Config::evaluate_next_strategy(int sim_id) {
    strategy_ = strategy_db_[sim_id + 1];
}

void Config::fix_pop_size_and_beta(std::vector<int> pop_size, std::vector<double> beta) {
    population_size_by_location_ = pop_size;
    beta_ = beta;
}
