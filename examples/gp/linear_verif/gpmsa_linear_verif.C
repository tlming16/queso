//-----------------------------------------------------------------------bl-
//--------------------------------------------------------------------------
//
// QUESO - a library to support the Quantification of Uncertainty
// for Estimation, Simulation and Optimization
//
// Copyright (C) 2008-2017 The PECOS Development Team
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the Version 2.1 GNU Lesser General
// Public License as published by the Free Software Foundation.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc. 51 Franklin Street, Fifth Floor,
// Boston, MA  02110-1301  USA
//
//-----------------------------------------------------------------------el-

// Bayes linear verification problems due to Brian Williams
//
// Described in: [1] Adams, B.M., Coleman, Kayla, Hooper, Russell W.,
//   Khuwaileh, Bassam A., Lewis, Allison, Smith, Ralph C., Swiler,
//   Laura P., Turinsky, Paul J., Williams, Brian J., "User Guidelines
//   and Best Practices for CASL VUQ Analysis Using Dakota, Sandia
//   Technical Report 2016-11614 (also CASL-U-2016-1233-000),
//   September 2016.
//
// Original QUESO/GPMSA example created by Brian Adams, 20170505

#include <queso/GslVector.h>
#include <queso/GslMatrix.h>
#include <queso/UniformVectorRV.h>
#include <queso/StatisticalInverseProblem.h>
#include <queso/VectorSet.h>
#include <queso/GPMSA.h>

#include <cstdio>

#define TOL 1e-5

void open_data_file(const std::string& data_filename, std::ifstream& data_stream) {
  std::string example_dir = "gp/linear_verify";
  data_stream.open(example_dir + "/" + data_filename);
  if (!data_stream.good()) {
    data_stream.open(data_filename);
    if (!data_stream.good())
      queso_error_msg(std::string("Cannot open data file: ") + data_filename);
  }
  data_stream.exceptions (std::ifstream::badbit | std::ifstream::eofbit |
      std::ifstream::failbit);
}

// Read in data files
void readData
(const std::string& sim_data_filename, const std::string& exp_data_filename,
 const std::vector<QUESO::SharedPtr<QUESO::GslVector>::Type> & simulationScenarios,
 const std::vector<QUESO::SharedPtr<QUESO::GslVector>::Type> & simulationParameters,
 const std::vector<QUESO::SharedPtr<QUESO::GslVector>::Type> & simulationOutputs,
 const std::vector<QUESO::SharedPtr<QUESO::GslVector>::Type> & experimentScenarios,
 const std::vector<QUESO::SharedPtr<QUESO::GslVector>::Type> & experimentOutputs,
 QUESO::GslMatrix& experimentMat)
{
  unsigned int num_config = simulationScenarios[0]->sizeGlobal();
  unsigned int num_params = simulationParameters[0]->sizeGlobal();
  unsigned int num_responses = simulationOutputs[0]->sizeGlobal();
  unsigned int num_simulations = simulationOutputs.size();

  // File containing x, theta, y (vertical concatenation of lhs.txt, y_mod.txt)
  std::ifstream sim_data;
  open_data_file(sim_data_filename, sim_data);
  try {
    for (unsigned int i = 0; i < num_simulations; ++i) {
      for (unsigned int j = 0; j < num_config; ++j)
        sim_data >> (*(simulationScenarios[i]))[j];
      for (unsigned int j = 0; j < num_params; ++j)
        sim_data >> (*(simulationParameters[i]))[j];
      for (unsigned int j = 0; j < num_responses; ++j) {
        sim_data >> (*(simulationOutputs[i]))[j];
      }
    }
  }
  catch (const std::ifstream::failure& e) {
    queso_error_msg(std::string("Error reading ") + sim_data_filename + ": " +
        e.what());
  }

  // Read in experimental data
  unsigned int num_experiments = experimentOutputs.size();
  std::ifstream exp_data;
  open_data_file(exp_data_filename, exp_data);
  try {
    for (unsigned int i = 0; i < num_experiments; ++i) {
      for (unsigned int j = 0; j < num_config; ++j)
        exp_data >> (*(experimentScenarios[i]))[j];
      for (unsigned int j = 0; j < num_responses; ++j)
        exp_data >> (*(experimentOutputs[i]))[j];
    }
  }
  catch (const std::ifstream::failure& e) {
    queso_error_msg(std::string("Error reading ") + exp_data_filename + ": " +
        e.what());
  }
}

void run_scalar(const QUESO::FullEnvironment& env)
{
  // Step 0: Set up some variables
  unsigned int numExperiments = 5;  // Number of experiments
  unsigned int numUncertainVars = 3;  // Number of things to calibrate (3 beta)
  unsigned int numSimulations = 60;  // Number of simulations
  unsigned int numConfigVars = 3;  // Dimension of configuration space (3 x)
  unsigned int numEta = 1;  // Number of responses the model is returning
  unsigned int experimentSize = 1;  // Size of each experiment

  // Step 2: Set up prior for calibration parameters
  QUESO::VectorSpace<QUESO::GslVector, QUESO::GslMatrix> paramSpace(env,
      "param_", numUncertainVars, NULL);

  // Parameter (theta) bounds:
  //   descriptors   'beta0'  'beta1'  'beta2'
  //   upper_bounds    0.45   -0.1      0.4
  //   lower_bounds   -0.1    -0.5     -0.2
  QUESO::GslVector paramMins(paramSpace.zeroVector());
  paramMins[0] = -0.1;
  paramMins[1] = -0.5;
  paramMins[2] = -0.2;

  QUESO::GslVector paramMaxs(paramSpace.zeroVector());
  paramMaxs[0] =  0.45;
  paramMaxs[1] = -0.1;
  paramMaxs[2] =  0.4;

  QUESO::BoxSubset<QUESO::GslVector, QUESO::GslMatrix> paramDomain("param_",
      paramSpace, paramMins, paramMaxs);

  QUESO::UniformVectorRV<QUESO::GslVector, QUESO::GslMatrix> priorRv("prior_",
      paramDomain);

  // Step 3: Instantiate the 'scenario' and 'output' spaces for simulation

  // Config space:
  //         Min  Max
  // x1      0.9   1.1
  // x2     -1.5   1.0
  // x3     -1.5   0.5
  QUESO::VectorSpace<QUESO::GslVector, QUESO::GslMatrix> configSpace(env,
      "scenario_", numConfigVars, NULL);

  QUESO::VectorSpace<QUESO::GslVector, QUESO::GslMatrix> nEtaSpace(env,
      "output_", numEta, NULL);

  // Step 4: Instantiate the 'output' space for the experiments
  QUESO::VectorSpace<QUESO::GslVector, QUESO::GslMatrix> experimentSpace(env,
      "experimentspace_", experimentSize, NULL);

  QUESO::VectorSpace<QUESO::GslVector, QUESO::GslMatrix> totalExperimentSpace(env,
      "experimentspace_", experimentSize * numExperiments, NULL);

  // Step 5: Instantiate the Gaussian process emulator object

  // GPMSA stores all the information about our simulation data and experimental
  // data, and if the users opts to, will normalise this to have zero mean and
  // unit variance.  It also stores default information about the hyperparameter
  // distributions.
  QUESO::GPMSAFactory<QUESO::GslVector, QUESO::GslMatrix>
    gpmsaFactory(env,
                 NULL,
                 priorRv,
                 configSpace,
                 paramSpace,
                 nEtaSpace,
                 numSimulations,
                 numExperiments);

  QUESO::GPMSAOptions& gp_opts = gpmsaFactory.options();

  // Must set scaling before adding experiments due to construct order
  // As of this implementation autoscale only affects params/scenarios

  // std::vector containing all the points in scenario space where we have
  // simulations
  std::vector<QUESO::SharedPtr<QUESO::GslVector>::Type>
    simulationScenarios(numSimulations);

  // std::vector containing all the points in parameter space where we have
  // simulations
  std::vector<QUESO::SharedPtr<QUESO::GslVector>::Type>
    paramVecs(numSimulations);

  // std::vector containing all the simulation output data
  std::vector<QUESO::SharedPtr<QUESO::GslVector>::Type>
    outputVecs(numSimulations);

  // std::vector containing all the points in scenario space where we have
  // experiments
  std::vector<QUESO::SharedPtr<QUESO::GslVector>::Type>
    experimentScenarios(numExperiments);

  // std::vector containing all the experimental output data
  std::vector<QUESO::SharedPtr<QUESO::GslVector>::Type>
    experimentVecs(numExperiments);

  // The experimental output data observation error covariance matrix
  QUESO::SharedPtr<QUESO::GslMatrix>::Type experimentMat
    (new QUESO::GslMatrix(totalExperimentSpace.zeroVector()));

  // Instantiate each of the simulation points/outputs
  for (unsigned int i = 0; i < numSimulations; i++) {
    simulationScenarios[i].reset(new QUESO::GslVector(configSpace.zeroVector()));  // 'x_{i+1}^*' in paper
    paramVecs          [i].reset(new QUESO::GslVector(paramSpace.zeroVector()));  // 't_{i+1}^*' in paper
    outputVecs         [i].reset(new QUESO::GslVector(nEtaSpace.zeroVector()));  // 'eta_{i+1}' in paper
  }

  // Must set scaling before adding experiments due to construct order
  for (unsigned int i = 0; i < numUncertainVars; i++)
    gp_opts.set_autoscale_minmax_uncertain_parameter(i);
  for (unsigned int i = 0; i < numConfigVars; i++)
    gp_opts.set_autoscale_minmax_scenario_parameter(i);
  for (unsigned int i = 0; i < numEta; i++)
    gp_opts.set_autoscale_meanvar_output(i);


  for (unsigned int i = 0; i < numExperiments; i++) {
    experimentScenarios[i].reset(new QUESO::GslVector(configSpace.zeroVector())); // 'x_{i+1}' in paper
    experimentVecs     [i].reset(new QUESO::GslVector(experimentSpace.zeroVector()));
  }

  // Experiment observation error (also needs to be scaled by
  // simulation variance)
  // NOTE: In general, this matrix would be size sum(experiment sizes)
  for (unsigned int i = 0; i < numExperiments; i++)
    (*experimentMat)(i, i) = std::pow(0.05, 2.0);

  // Read in data and store the standard deviation of the simulation
  // data (ignored for now).
  readData("sim_scalar.dat",
           "y_exp_scalar.txt",
           simulationScenarios,
           paramVecs,
           outputVecs,
           experimentScenarios,
           experimentVecs,
           *experimentMat);

  // Add simulation and experimental data
  gpmsaFactory.addSimulations(simulationScenarios, paramVecs, outputVecs);
  gpmsaFactory.addExperiments(experimentScenarios, experimentVecs, experimentMat);

  QUESO::GenericVectorRV<> postRv(
      "post_",
      gpmsaFactory.prior().imageSet().vectorSpace());

  QUESO::StatisticalInverseProblem<QUESO::GslVector, QUESO::GslMatrix> ip(
      "",
      NULL,
      gpmsaFactory,
      postRv);

  QUESO::GslVector paramInitials(
      gpmsaFactory.prior().imageSet().vectorSpace().zeroVector());

   // Start with the mean of the prior
   gpmsaFactory.prior().pdf().distributionMean(paramInitials);

   // Initial condition of the chain: may need to tweak to Brian's expectations
   std::cout << "\nPrior-based initial position:\n" << paramInitials << std::endl;

   // Brian Williams' recommended initial point
   paramInitials[0]  = 0.175;    // beta0
   paramInitials[1]  = -0.3;     // beta1
   paramInitials[2]  = 0.1;      // beta2
   // [ truncation error precision ] (truncated SVD case only)
   // BJW: max(100, shape * scale)
   paramInitials[3]  = 1.0;               // emul precision
   paramInitials[4]  = std::exp(-0.025);  // emul corr strength (scenario/beta?)
   paramInitials[5]  = std::exp(-0.025);  // emul corr strength (scenario/beta?)
   paramInitials[6]  = std::exp(-0.025);  // emul corr strength (scenario/beta?)
   paramInitials[7]  = std::exp(-0.025);  // emul corr strength (scenario/beta?)
   paramInitials[8]  = std::exp(-0.025);  // emul corr strength (scenario/beta?)
   paramInitials[9]  = std::exp(-0.025);  // emul corr strength (scenario/beta?)
   paramInitials[10] = 20.0;              // discrepancy precision
   paramInitials[11] = std::exp(-0.025);  // discrepancy corr strength x1
   paramInitials[12] = std::exp(-0.025);  // discrepancy corr strength x2
   paramInitials[13] = std::exp(-0.025);  // discrepancy corr strength x3
   paramInitials[14] = 1000.0;            // emul data precision
   if (gpmsaFactory.options().m_calibrateObservationalPrecision)
     // BJW: max(20, shape * scale)
     paramInitials[15] = 20.0;            // obs precision

   std::cout << "\nAdjusted initial position:\n" << paramInitials << std::endl;

   QUESO::GslMatrix proposalCovMatrix(
       gpmsaFactory.prior().imageSet().vectorSpace().zeroVector());

   // Start with the covariance matrix for the whole prior, including
   // GPMSA hyper-parameters.
   gpmsaFactory.prior().pdf().distributionVariance(proposalCovMatrix);

   std::cout << "\nPrior proposal covariance diagonal:\n";
   for (unsigned int i=0; i<proposalCovMatrix.numCols(); ++i)
     std::cout << proposalCovMatrix(i,i) << " ";
   std::cout << std::endl;

   // FIXME: The default doesn't seem to work for this case; fudge it:
   proposalCovMatrix(10, 10) = 1.0e2;
   proposalCovMatrix(14, 14) = 1.0e1;

   std::cout << "\nFinal proposal covariance diagonal:\n";
   for (unsigned int i=0; i<proposalCovMatrix.numCols(); ++i)
     std::cout << proposalCovMatrix(i,i) << " ";
   std::cout << std::endl;

   std::cout << "\nFinal GPMSA Options:" << gpmsaFactory.options() << std::endl;

   ip.solveWithBayesMetropolisHastings(NULL, paramInitials, &proposalCovMatrix);
}


void run_multivariate(const QUESO::FullEnvironment& env)
{
  // Step 0: Set up some variables
  unsigned int numExperiments = 5;  // Number of experiments
  unsigned int numUncertainVars = 3;  // Number of things to calibrate (3 beta)
  unsigned int numSimulations = 60;  // Number of simulations
  unsigned int numConfigVars = 3;  // Dimension of configuration space (3 x)
  unsigned int numEta = 5;  // Number of responses the model is returning
  unsigned int experimentSize = 5;  // Size of each experiment

  // Step 2: Set up prior for calibration parameters
  QUESO::VectorSpace<QUESO::GslVector, QUESO::GslMatrix> paramSpace(env,
      "param_", numUncertainVars, NULL);

  // Parameter (theta) bounds:
  //   descriptors   'beta0'  'beta1'  'beta2'
  //   upper_bounds    0.45   -0.1      0.4
  //   lower_bounds   -0.1    -0.5     -0.2
  QUESO::GslVector paramMins(paramSpace.zeroVector());
  paramMins[0] = -0.1;
  paramMins[1] = -0.5;
  paramMins[2] = -0.2;

  QUESO::GslVector paramMaxs(paramSpace.zeroVector());
  paramMaxs[0] =  0.45;
  paramMaxs[1] = -0.1;
  paramMaxs[2] =  0.4;

  QUESO::BoxSubset<QUESO::GslVector, QUESO::GslMatrix> paramDomain("param_",
      paramSpace, paramMins, paramMaxs);

  QUESO::UniformVectorRV<QUESO::GslVector, QUESO::GslMatrix> priorRv("prior_",
      paramDomain);

  // Step 3: Instantiate the 'scenario' and 'output' spaces for simulation

  // Config space:
  //         Min  Max
  // x1      0.9   1.1
  // x2     -1.5   1.0
  // x3     -1.5   0.5

  QUESO::VectorSpace<QUESO::GslVector, QUESO::GslMatrix> configSpace(env,
      "scenario_", numConfigVars, NULL);

  QUESO::VectorSpace<QUESO::GslVector, QUESO::GslMatrix> nEtaSpace(env,
      "output_", numEta, NULL);

  // Step 4: Instantiate the 'output' space for the experiments
  QUESO::VectorSpace<QUESO::GslVector, QUESO::GslMatrix> experimentSpace(env,
      "experimentspace_", experimentSize, NULL);

  QUESO::VectorSpace<QUESO::GslVector, QUESO::GslMatrix> totalExperimentSpace(env,
      "experimentspace_", experimentSize * numExperiments, NULL);

  // Step 5: Instantiate the Gaussian process emulator object
  //
  // Regarding simulation scenario input values, the user should standardise
  // them so that they exist inside a hypercube.
  //
  // Regarding simulation output data, the user should transform it so that the
  // mean is zero and the variance is one.
  //
  // Regarding experimental scenario input values, the user should standardize
  // them so that they exist inside a hypercube.
  //
  // Regarding experimental data, the user should transformed it so that it has
  // zero mean and variance one.

  // GPMSA stores all the information about our simulation
  // data and experimental data.  It also stores default information about the
  // hyperparameter distributions.
  QUESO::GPMSAFactory<QUESO::GslVector, QUESO::GslMatrix>
    gpmsaFactory(env,
                 NULL,
                 priorRv,
                 configSpace,
                 paramSpace,
                 nEtaSpace,
                 numSimulations,
                 numExperiments);

  QUESO::GPMSAOptions& gp_opts = gpmsaFactory.options();

  // Must set scaling before adding experiments due to construct order
  // As of this implementation autoscale only affects params/scenarios

  // TODO: Ask Brian Williams about preferred scaling and probably set
  // to scale to domain bounds instead of data bounds...
  gp_opts.set_autoscale_minmax();

  // std::vector containing all the points in scenario space where we have
  // simulations
  std::vector<QUESO::SharedPtr<QUESO::GslVector>::Type>
    simulationScenarios(numSimulations);

  // std::vector containing all the points in parameter space where we have
  // simulations
  std::vector<QUESO::SharedPtr<QUESO::GslVector>::Type>
    paramVecs(numSimulations);

  // std::vector containing all the simulation output data
  std::vector<QUESO::SharedPtr<QUESO::GslVector>::Type>
    outputVecs(numSimulations);

  // std::vector containing all the points in scenario space where we have
  // experiments
  std::vector<QUESO::SharedPtr<QUESO::GslVector>::Type>
    experimentScenarios(numExperiments);

  // std::vector containing all the experimental output data
  std::vector<QUESO::SharedPtr<QUESO::GslVector>::Type>
    experimentVecs(numExperiments);

  // The experimental output data observation error covariance matrix
  QUESO::SharedPtr<QUESO::GslMatrix>::Type experimentMat
    (new QUESO::GslMatrix(totalExperimentSpace.zeroVector()));

  // Instantiate each of the simulation points/outputs
  for (unsigned int i = 0; i < numSimulations; i++) {
    simulationScenarios[i].reset(new QUESO::GslVector(configSpace.zeroVector()));  // 'x_{i+1}^*' in paper
    paramVecs          [i].reset(new QUESO::GslVector(paramSpace.zeroVector()));  // 't_{i+1}^*' in paper
    outputVecs         [i].reset(new QUESO::GslVector(nEtaSpace.zeroVector()));  // 'eta_{i+1}' in paper
  }

  for (unsigned int i = 0; i < numExperiments; i++) {
    experimentScenarios[i].reset(new QUESO::GslVector(configSpace.zeroVector())); // 'x_{i+1}' in paper
    experimentVecs     [i].reset(new QUESO::GslVector(experimentSpace.zeroVector()));
  }


  // Experiment observation error

  // True observation error for each experiment uses this R among responses
  //   0.0025   0.002   0.0016 0.00128 0.001024
  //   0.002    0.0025  0.002  0.0016  0.00128
  //   0.0016   0.002   0.0025 0.002   0.0016
  //   0.00128  0.0016  0.002  0.0025  0.002
  //   0.001024 0.00128 0.0016 0.002   0.0025
  QUESO::GslMatrix covarianceR(experimentSpace.zeroVector());
  for (unsigned int i = 0; i < 5; ++i)
    covarianceR(i, i) = 0.0025;
  for (unsigned int i = 0; i < 4; ++i)
    covarianceR(i, i+1) = covarianceR(i+1, i) = 0.002;
  for (unsigned int i = 0; i < 3; ++i)
    covarianceR(i, i+2) = covarianceR(i+2, i) = 0.0016;
  for (unsigned int i = 0; i < 2; ++i)
    covarianceR(i, i+3) = covarianceR(i+3, i) = 0.00128;
  covarianceR(0,4) = covarianceR(4,0) = 0.001024;

  // Populate the totalExperimentSpace covariance matrix
  std::vector<const QUESO::GslMatrix* > vec_covmat_ptrs(numExperiments,
      &covarianceR);
  experimentMat->fillWithBlocksDiagonally(0, 0, vec_covmat_ptrs, true, true);

  // Read in data and store the standard deviation of the simulation
  // data (ignored for now).
  //double stdsim =
  readData("sim_mv.dat",
           "y_exp_mv.txt",
           simulationScenarios,
           paramVecs,
           outputVecs,
           experimentScenarios,
           experimentVecs,
           *experimentMat);

  // Add simulation and experimental data
  gpmsaFactory.addSimulations(simulationScenarios, paramVecs, outputVecs);
  gpmsaFactory.addExperiments(experimentScenarios, experimentVecs, experimentMat);

  QUESO::GenericVectorRV<QUESO::GslVector, QUESO::GslMatrix> postRv(
      "post_",
      gpmsaFactory.prior().imageSet().vectorSpace());
  QUESO::StatisticalInverseProblem<QUESO::GslVector, QUESO::GslMatrix> ip("",
      NULL, gpmsaFactory, postRv);

  QUESO::GslVector paramInitials(
      gpmsaFactory.prior().imageSet().vectorSpace().zeroVector());

  // Start with the mean of the prior
  gpmsaFactory.prior().pdf().distributionMean(paramInitials);

  // Initial condition of the chain: may need to tweak to Brian's expectations
  std::cout << "\nPrior-based initial position:\n" << paramInitials << std::endl;

  // Brian Williams' recommended initial point
  paramInitials[0]  = 0.175;    // beta0
  paramInitials[1]  = -0.3;     // beta1
  paramInitials[2]  = 0.1;      // beta2
  // [ truncation error precision ] (truncated SVD case only)
  paramInitials[3]  = 1.0;               // emul precision
  paramInitials[4]  = 1.0;               // weights0 precision
  paramInitials[5]  = 1.0;               // weights0 precision
  paramInitials[6]  = 1.0;               // weights0 precision
  paramInitials[7]  = 1.0;               // weights0 precision
  paramInitials[8]  = 1.0;               // weights0 precision
  paramInitials[9]  = std::exp(-0.025);  // emul corr strength (scenario/beta?)
  paramInitials[10] = std::exp(-0.025);  // emul corr strength (scenario/beta?)
  paramInitials[11] = std::exp(-0.025);  // emul corr strength (scenario/beta?)
  paramInitials[12] = std::exp(-0.025);  // emul corr strength (scenario/beta?)
  paramInitials[13] = std::exp(-0.025);  // emul corr strength (scenario/beta?)
  paramInitials[14] = std::exp(-0.025);  // emul corr strength (scenario/beta?)
  paramInitials[15] = 20.0;              // discrepancy precision
  paramInitials[16] = std::exp(-0.025);  // discrepancy corr strength x1
  paramInitials[17] = std::exp(-0.025);  // discrepancy corr strength x2
  paramInitials[18] = std::exp(-0.025);  // discrepancy corr strength x3
  paramInitials[19] = 1000.0;            // emul data precision
  if (gpmsaFactory.options().m_calibrateObservationalPrecision)
    // BJW: max(20, shape * scale)
    paramInitials[20] = 20.0;   // obs precision

  std::cout << "\nAdjusted initial position:\n" << paramInitials << std::endl;

  QUESO::GslMatrix proposalCovMatrix(
      gpmsaFactory.prior().imageSet().vectorSpace().zeroVector());

  // Start with the covariance matrix for the whole prior, including
  // GPMSA hyper-parameters.
  gpmsaFactory.prior().pdf().distributionVariance(proposalCovMatrix);

  std::cout << "\nPrior proposal covariance diagonal:\n";
  for (unsigned int i=0; i<proposalCovMatrix.numCols(); ++i)
    std::cout << proposalCovMatrix(i,i) << " ";
  std::cout << std::endl;

  // FIXME: The default doesn't seem to work for this case; fudge it:
  proposalCovMatrix(15, 15) = 1.0e2;
  proposalCovMatrix(19, 19) = 1.0e1;

  std::cout << "\nFinal proposal covariance diagonal:\n";
  for (unsigned int i=0; i<proposalCovMatrix.numCols(); ++i)
    std::cout << proposalCovMatrix(i,i) << " ";
  std::cout << std::endl;

  std::cout << "\nFinal GPMSA Options:" << gpmsaFactory.options() << std::endl;

  ip.solveWithBayesMetropolisHastings(NULL, paramInitials, &proposalCovMatrix);
}


int main(int argc, char ** argv)
{
  if (argc < 2) {
    std::cerr << "Usage: argv[0] gpmsa_<case>.txt\n";
    return 1;
  }

#ifdef QUESO_HAS_MPI
  MPI_Init(&argc, &argv);

  // Step 1: Set up QUESO environment
  QUESO::FullEnvironment env(MPI_COMM_WORLD, argv[1], "", NULL);
#else
  QUESO::FullEnvironment env(argv[1], "", NULL);
#endif

  std::string input_file(argv[1]);
  if (input_file.find("scalar") != std::string::npos)
    run_scalar(env);
  else if  (input_file.find("mv") != std::string::npos)
    run_multivariate(env);
  else {
    std::cerr << "Unknown case with input: " << input_file <<"\n";
    return 1;
  }

#ifdef QUESO_HAS_MPI
  MPI_Finalize();
#endif

  return 0;
}
