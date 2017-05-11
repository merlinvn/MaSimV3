/* 
 * File:   main.cpp
 * Author: Merlin
 *
 * Created on October 29, 2014, 9:51 AM
 */

#include <cstdlib>
#include "Model.h"
#include "Random.h"
#include "ezOptionParser.hpp"
#include <boost/foreach.hpp>
#include "Reporter.h"
#include "HelperFunction.h"
#include "PkPdReporter.h"
#include "IntGenotype.h"
#include "IntGenotypeDatabase.h"
#include "Config.h"
#include "Population.h"
#include "PersonIndexAll.h"
#include "ClonalParasitePopulation.h"
#include "ProgressToClinicalEvent.h"
#include "ImmuneSystem.h"
#include "IndividualsFileReporter.h"
#include <string>
#include "Strategy.h"
#include "SCTherapy.h"

using namespace std;
void CreateOptionParser(ez::ezOptionParser& opt);
void handle_option_parser(Model* m, int argc, const char * argv[]);

void Usage(ez::ezOptionParser& opt) {
    std::string usage;
    opt.getUsage(usage);
    std::cout << usage;
};

std::string individuals_output_file = "individuals.txt";
int genotype_id = 0;
int dosing_day = 3;

/*
 * 
 */
int main(int argc, const char * argv[]) {
    Model* m = new Model();
    handle_option_parser(m, argc, argv);

    m->initialize();

    BOOST_FOREACH(Reporter* reporter, m->reporters()) {
        DeletePointer<Reporter>(reporter);
    }
    m->reporters().clear();
    m->add_reporter(new PkPdReporter());
    m->add_reporter(new IndividualsFileReporter(individuals_output_file));

    //infect all individual and schedule progress to clinical at day 0
    //    IntGenotype* genotype = Model::CONFIG->genotype_db()->get(12);
    IntGenotype* genotype = Model::CONFIG->genotype_db()->get(genotype_id);

    for (int i = 0; i < Model::CONFIG->strategy()->therapy_list().size(); i++) {
        SCTherapy* scTherapy = dynamic_cast<SCTherapy*> (Model::CONFIG->strategy()->therapy_list()[i]);
        if (scTherapy != NULL) {
            scTherapy->set_dosing_day(dosing_day);
        }
    }

    //    std::cout << Model::CONFIG->drug_db()->get(1)->get_parasite_killing_rate_by_concentration(1, genotype) << std::endl;
    //    std::cout << Model::CONFIG->drug_db()->get(2)->get_parasite_killing_rate_by_concentration(1, genotype) << std::endl;
    //    std::cout << Model::CONFIG->drug_db()->get(3)->get_parasite_killing_rate_by_concentration(1, genotype) << std::endl;
    //
    //    std::cout << Model::CONFIG->drug_db()->get(1)->get_parasite_killing_rate_by_concentration(0.9, genotype) << std::endl;
    //    std::cout << Model::CONFIG->drug_db()->get(2)->get_parasite_killing_rate_by_concentration(0.9, genotype) << std::endl;
    //    std::cout << Model::CONFIG->drug_db()->get(3)->get_parasite_killing_rate_by_concentration(0.9, genotype) << std::endl;
    //
    //    std::cout << Model::CONFIG->drug_db()->get(1)->get_parasite_killing_rate_by_concentration(0.8, genotype) << std::endl;
    //    std::cout << Model::CONFIG->drug_db()->get(2)->get_parasite_killing_rate_by_concentration(0.8, genotype) << std::endl;
    //    std::cout << Model::CONFIG->drug_db()->get(3)->get_parasite_killing_rate_by_concentration(0.8, genotype) << std::endl;
    // 
    //    std::cout << Model::CONFIG->drug_db()->get(1)->get_parasite_killing_rate_by_concentration(0.7, genotype) << std::endl;
    //    std::cout << Model::CONFIG->drug_db()->get(2)->get_parasite_killing_rate_by_concentration(0.7, genotype) << std::endl;
    //    std::cout << Model::CONFIG->drug_db()->get(3)->get_parasite_killing_rate_by_concentration(0.7, genotype) << std::endl;

    //    double density = Model::CONFIG->log_parasite_density_level().log_parasite_density_clinical;

    BOOST_FOREACH(Person* person, Model::POPULATION->all_persons()->vPerson()) {
        double density = Model::RANDOM->random_uniform_double(Model::CONFIG->log_parasite_density_level().log_parasite_density_clinical_from, Model::CONFIG->log_parasite_density_level().log_parasite_density_clinical_to);
        ClonalParasitePopulation* blood_parasite = person->add_new_parasite_to_blood(genotype);
        //        BloodParasite* blood_parasite1 = person->add_new_parasite_to_blood(genotype);
        //        BloodParasite* blood_parasite2 = person->add_new_parasite_to_blood(genotype);
        person->immune_system()->set_increase(true);
        person->set_host_state(Person::EXPOSED);
//            std::cout << "hello"<< std::endl;
        blood_parasite->set_gametocyte_level(Model::CONFIG->gametocyte_level_full());
        blood_parasite->set_last_update_log10_parasite_density(density);
        //        blood_parasite1->set_last_update_log10_parasite_density(density);
        //        blood_parasite2->set_last_update_log10_parasite_density(density);

        ProgressToClinicalEvent::schedule_event(Model::SCHEDULER, person, blood_parasite, 1);
    }



    m->run();
    delete m;
    m = NULL;
    return 0;
}

void CreateOptionParser(ez::ezOptionParser& opt) {
    opt.overview = "PK/PD simulation for malaria.";
    opt.syntax = "PkPdSimulation [OPTIONS]";
    opt.example = "PkPdSimulation -h\n\n";
    opt.footer = "PkPdSimulation v1.0.0 Copyright (C) 2014 OUCRU.\n";

    opt.add(
            "0", // Default.
            0, // Required?
            1, // Number of args expected.
            0, // Delimiter if expecting multiple args.
            "Display usage instructions.\nThere is a choice of three different layouts for description alignment. Your choice can be any one of the following to suit your style:\n\n0 - align (default)\n1 - interleave\n2 - stagger", // Help description.
            "-h", // Flag token. 
            "-help", // Flag token.
            "--help", // Flag token.
            "--usage" // Flag token.
            );


    opt.add(
            "basic_config.yml", // Default.
            0, // Required?
            1, // Number of args expected.
            0, // Delimiter if expecting multiple args.
            "The config file (YAML format).", // Help description.
            "-c", // Flag token. 
            "-i", // stand for input
            "-cfg", // Flag token.
            "--config" // Flag token.
            );


    opt.add(
            "basic_config.yml", // Default.
            0, // Required?
            1, // Number of args expected.
            0, // Delimiter if expecting multiple args.
            "The individuals parasite density by day", // Help description.
            "-o", // Flag token. 
            "-output", // Flag token.
            "--output" // Flag token.
            );
    opt.add("0",
            0,
            1,
            0,
            "Initial Genotype_id",
            "-genotype_id",
            "--genotype_id"
            );
    opt.add("3",
            0,
            1,
            0,
            "Dosing day",
            "-dosing_day",
            "--dosing_day"
            );
    opt.add("-1",
            0,
            1,
            0,
            "Seed number for random generator",
            "-s",
            "--seed"
            );

}

void handle_option_parser(Model* m, int argc, const char * argv[]) {
    ez::ezOptionParser opt;
    CreateOptionParser(opt);
    opt.parse(argc, argv);

    if (opt.isSet("-h")) {
        std::string usage;
        int layout;
        opt.get("-h")->getInt(layout);
        switch (layout) {
            case 0:
                opt.getUsage(usage, 80, ez::ezOptionParser::ALIGN);
                break;
            case 1:
                opt.getUsage(usage, 80, ez::ezOptionParser::INTERLEAVE);
                break;
            case 2:
                opt.getUsage(usage, 80, ez::ezOptionParser::STAGGER);
                break;
        }
        std::cout << usage;
        exit(1);
    }

    std::vector<std::string> badOptions;
    int i;
    if (!opt.gotRequired(badOptions)) {
        for (i = 0; i < badOptions.size(); ++i)
            std::cerr << "ERROR: Missing required option " << badOptions[i] << ".\n\n";
        Usage(opt);
        exit(1);
    }
    std::string config_filename = "basic_config.yml";
    if (opt.isSet("-c")) {

        opt.get("-c")->getString(config_filename);
        //        std::cout << config_filename << std::endl;
    }
    m->set_config_filename(config_filename);


    long seed = -1;
    if (opt.isSet("-s")) {
        opt.get("-s")->getLong(seed);
        //        std::cout << seed << std::endl;
    }
    m->set_initial_seed_number(seed);

    if (opt.isSet("-o")) {
        opt.get("-o")->getString(individuals_output_file);
    }
    if (opt.isSet("-genotype_id")) {
        opt.get("-genotype_id")->getInt(genotype_id);
    }
    if (opt.isSet("-dosing_day")) {
        opt.get("-dosing_day")->getInt(dosing_day);
    }
}
