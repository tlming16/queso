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

#include <queso/Defines.h>

#ifndef QUESO_DISABLE_BOOST_PROGRAM_OPTIONS
#include <boost/program_options.hpp>
#else
#define GETPOT_NAMESPACE QUESO // So we don't clash with other getpots
#include <queso/getpot.h>
#undef GETPOT_NAMESPACE
#endif  // QUESO_DISABLE_BOOST_PROGRAM_OPTIONS

#include <queso/StatisticalInverseProblemOptions.h>
#include <queso/Miscellaneous.h>

// -------------------------------------------------
// SipOptionsValues--------------------------
// -------------------------------------------------

namespace QUESO {

// Default constructor -----------------------------
SipOptionsValues::SipOptionsValues()
#ifndef QUESO_DISABLE_BOOST_PROGRAM_OPTIONS
  : m_parser(new BoostInputOptionsParser())
#endif  // QUESO_DISABLE_BOOST_PROGRAM_OPTIONS
{
  this->set_defaults();
  this->set_prefix("");
}

SipOptionsValues::SipOptionsValues(const BaseEnvironment* env,
				   const char* prefix)
{
  this->set_defaults();
  this->parse(*env, prefix);
}

// Copy constructor - -----------------------------
SipOptionsValues::SipOptionsValues(const SipOptionsValues& src)
{
  this->copy(src);
}
// Destructor ---------------------------------------
SipOptionsValues::~SipOptionsValues()
{
}

// Set methods --------------------------------------
SipOptionsValues&
SipOptionsValues::operator=(const SipOptionsValues& rhs)
{
  this->copy(rhs);
  return *this;
}
// Private methods-----------------------------------
void
SipOptionsValues::checkOptions()
{
  // Nothing
}

void
SipOptionsValues::copy(const SipOptionsValues& src)
{
  m_computeSolution      = src.m_computeSolution;
  m_dataOutputFileName   = src.m_dataOutputFileName;
  m_dataOutputAllowedSet = src.m_dataOutputAllowedSet;
#ifdef UQ_SIP_READS_SOLVER_OPTION
  m_solverString         = src.m_solverString;
#endif
  m_seedWithMAPEstimator = src.m_seedWithMAPEstimator;
  m_useOptimizerMonitor = src.m_useOptimizerMonitor;
}

std::ostream &
operator<<(std::ostream& os, const SipOptionsValues & obj)
{
  os << "\n" << obj.m_option_computeSolution      << " = " << obj.m_computeSolution
     << "\n" << obj.m_option_dataOutputFileName   << " = " << obj.m_dataOutputFileName;
  os << "\n" << obj.m_option_dataOutputAllowedSet << " = ";
  for (std::set<unsigned int>::iterator setIt = obj.m_dataOutputAllowedSet.begin(); setIt != obj.m_dataOutputAllowedSet.end(); ++setIt) {
    os << *setIt << " ";
  }
#ifdef UQ_SIP_READS_SOLVER_OPTION
     << "\n" << obj.m_option_solver << " = " << obj.m_solverString
#endif
  os << std::endl;
  return os;
}


void
SipOptionsValues::set_defaults()
{
  m_help = UQ_SIP_HELP;
  m_computeSolution = UQ_SIP_COMPUTE_SOLUTION_ODV;
  m_dataOutputFileName = UQ_SIP_DATA_OUTPUT_FILE_NAME_ODV;
  m_seedWithMAPEstimator = UQ_SIP_SEEDWITHMAPESTIMATOR;
  m_useOptimizerMonitor = UQ_SIP_USEOPTIMIZERMONITOR;
//m_dataOutputAllowedSet()
#ifdef UQ_SIP_READS_SOLVER_OPTION
  m_solverString = UQ_SIP_SOLVER_ODV;
#endif
}

void
SipOptionsValues::set_prefix(const std::string& prefix)
{
  m_prefix = prefix + "ip_";

  m_option_help = m_prefix + "help";
  m_option_computeSolution = m_prefix + "computeSolution";
  m_option_dataOutputFileName = m_prefix + "dataOutputFileName";
  m_option_dataOutputAllowedSet = m_prefix + "dataOutputAllowedSet";
#ifdef UQ_SIP_READS_SOLVER_OPTION
  m_option_solver = m_prefix + "solver";
#endif
  m_option_seedWithMAPEstimator = m_prefix + "seedWithMAPEstimator";
  m_option_useOptimizerMonitor = m_prefix + "useOptimizerMonitor";
}

void
SipOptionsValues::parse(const BaseEnvironment& env, const std::string& prefix)
{
  this->set_prefix(prefix);

#ifndef QUESO_DISABLE_BOOST_PROGRAM_OPTIONS

  m_parser.reset(new BoostInputOptionsParser(env.optionsInputFileName()));

  m_parser->registerOption<std::string>
    (m_option_help, m_help,
    "produce help message for statistical inverse problem");
  m_parser->registerOption<bool>
    (m_option_computeSolution, m_computeSolution,
    "compute solution process");
  m_parser->registerOption<std::string>
    (m_option_dataOutputFileName, m_dataOutputFileName,
    "name of data output file");
  m_parser->registerOption<std::string>
    (m_option_dataOutputAllowedSet, container_to_string(m_dataOutputAllowedSet),
    "subEnvs that will write to data output file");
#ifdef UQ_SIP_READS_SOLVER_OPTION
  m_parser->registerOption<std::string>
    (m_option_solver, m_solver,
    "algorithm for calibration");
#endif
  m_parser->registerOption<bool>
    (m_option_seedWithMAPEstimator, m_seedWithMAPEstimator,
    "toggle for seeding chain at MAP");
  m_parser->registerOption<bool>
    (m_option_useOptimizerMonitor, m_useOptimizerMonitor,
    "toggle for using optimizer monitor (prints diagnostics");

  m_parser->scanInputFile();

  m_parser->getOption<std::string>(m_option_help, m_help);
  m_parser->getOption<bool>(m_option_computeSolution, m_computeSolution);
  m_parser->getOption<std::string>
    (m_option_dataOutputFileName, m_dataOutputFileName);
  m_parser->getOption<std::set<unsigned int> >
    (m_option_dataOutputAllowedSet, m_dataOutputAllowedSet);
#ifdef UQ_SIP_READS_SOLVER_OPTION
  m_parser->getOption<std::string>(m_option_solver, m_solver);
#endif
  m_parser->getOption<bool>
    (m_option_seedWithMAPEstimator, m_seedWithMAPEstimator);
  m_parser->getOption<bool>
    (m_option_useOptimizerMonitor, m_useOptimizerMonitor);

#else

  m_help = env.input()(m_option_help, m_help);
  m_computeSolution = env.input()(m_option_computeSolution, m_computeSolution);
  m_dataOutputFileName = env.input()
    (m_option_dataOutputFileName, m_dataOutputFileName);

  // UQ_SIP_DATA_OUTPUT_ALLOWED_SET_ODV is the empty set (string) by default
  unsigned int size =
    env.input().vector_variable_size(m_option_dataOutputAllowedSet);
  for (unsigned int i = 0; i < size; i++) {
    // We default to empty set, so the default values are actually never
    // used here
    unsigned int allowed = env.input()(m_option_dataOutputAllowedSet, i, i);
    m_dataOutputAllowedSet.insert(allowed);
  }

#ifdef UQ_SIP_READS_SOLVER_OPTION
  m_solver = env.input()(m_option_solver, m_solver);
#endif
  m_seedWithMAPEstimator = env.input()
    (m_option_seedWithMAPEstimator, m_seedWithMAPEstimator);
  m_useOptimizerMonitor = env.input()
    (m_option_useOptimizerMonitor, m_useOptimizerMonitor);
#endif  // QUESO_DISABLE_BOOST_PROGRAM_OPTIONS

  checkOptions();
}

}  // End namespace QUESO
