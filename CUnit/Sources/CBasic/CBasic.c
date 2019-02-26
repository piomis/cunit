/*
 *  CUnit - A Unit testing framework library for C.
 *  Copyright (C) 2004-2019  Jerry St.Clair, Anil Kumar, Piotr Mis
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 *  Implementation for basic test runner interface.
 *
 *  11-Aug-2004   Initial implementation of basic test runner interface.  (JDS)
 *
 *  8-Jan-2005    Fixed reporting bug (bug report cunit-Bugs-1093861).  (JDS)
 *
 *  30-Apr-2005   Added notification of suite cleanup failure.  (JDS)
 *
 *  02-May-2006   Added internationalization hooks.  (JDS)
 *
 *  26-Jan-2019   Created CBasic implementation for colourful version of Basic.  (PMi)
 */

/** @file
 * CBasic interface with output to stdout.
 */
/** @addtogroup CBasic
 * @{
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>

#define CBASIC_NO_ANSI (1)

#ifdef CBASIC_NO_ANSI
#include <windows.h>
#endif

#include "CUnit.h"
#include "TestDB.h"
#include "Util.h"
#include "TestRun.h"
#include "CBasic.h"
#include "CUnit_intl.h"

#ifdef CBASIC_NO_ANSI
#define CU_CBRM_FAIL_COLOUR      (FOREGROUND_RED | FOREGROUND_INTENSITY)
#define CU_CBRM_PASS_COLOUR      (FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#define CU_CBRM_NORMAL_COLOUR    (0x0F)
#define CU_CBRM_SKIP_COLOUR      (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#define CU_CBASIC_SET_COLOUR(_colour_)    SetConsoleTextAttribute(hConsole, (_colour_))
#else
#define CU_CBRM_FAIL_COLOUR       "\x1b[91m"
#define CU_CBRM_PASS_COLOUR       "\x1b[92m"
#define CU_CBRM_SKIP_COLOUR       "\x1b[93m"
#define CU_CBRM_NORMAL_COLOUR     "\x1b[0m"

#define CU_CBASIC_SET_COLOUR(_colour_)    fprintf(stdout,"%s", (_colour_))
#endif

/*=================================================================
 *  Global/Static Definitions
 *=================================================================*/
/** Pointer to the currently running suite. */
static CU_pSuite f_pRunningSuite = NULL;
/** Current run mode. */
static CU_CBasicRunMode f_run_mode = CU_CBRM_NORMAL;

#ifdef CBASIC_NO_ANSI
static HANDLE  hConsole;
#endif

/*=================================================================
 *  Forward declaration of module functions *
 *=================================================================*/
static CU_ErrorCode cbasic_initialize(void);
static CU_ErrorCode cbasic_run_all_tests(CU_pTestRegistry pRegistry);
static CU_ErrorCode cbasic_run_suite(CU_pSuite pSuite);
static CU_ErrorCode cbasic_run_single_test(CU_pSuite pSuite, CU_pTest pTest);

static void cbasic_test_start_message_handler(const CU_pTest pTest, const CU_pSuite pSuite);
static void cbasic_test_complete_message_handler(const CU_pTest pTest, const CU_pSuite pSuite, const CU_pFailureRecord pFailureList);
static void cbasic_all_tests_complete_message_handler(const CU_pFailureRecord pFailure);
static void cbasic_suite_init_failure_message_handler(const CU_pSuite pSuite);
static void cbasic_suite_cleanup_failure_message_handler(const CU_pSuite pSuite);

/*=================================================================
 *  Public Interface functions
 *=================================================================*/
CU_ErrorCode CU_cbasic_run_tests(void)
{
  CU_ErrorCode error;

  if (NULL == CU_get_registry()) {
    if (CU_CBRM_SILENT != f_run_mode)
      fprintf(stderr, "\n\n%s\n", _("FATAL ERROR - Test registry is not initialized."));
    error = CUE_NOREGISTRY;
  }
  else if (CUE_SUCCESS == (error = cbasic_initialize()))
    error = cbasic_run_all_tests(NULL);

  return error;
}

/*------------------------------------------------------------------------*/
CU_ErrorCode CU_cbasic_run_suite(CU_pSuite pSuite)
{
  CU_ErrorCode error;

  if (NULL == pSuite)
    error = CUE_NOSUITE;
  else if (CUE_SUCCESS == (error = cbasic_initialize()))
    error = cbasic_run_suite(pSuite);

  return error;
}

/*------------------------------------------------------------------------*/
CU_ErrorCode CU_cbasic_run_test(CU_pSuite pSuite, CU_pTest pTest)
{
  CU_ErrorCode error;

  if (NULL == pSuite)
    error = CUE_NOSUITE;
  else if (NULL == pTest)
    error = CUE_NOTEST;
  else if (CUE_SUCCESS == (error = cbasic_initialize()))
    error = cbasic_run_single_test(pSuite, pTest);

  return error;
}

/*------------------------------------------------------------------------*/
void CU_cbasic_set_mode(CU_CBasicRunMode mode)
{
  f_run_mode = mode;
}

/*------------------------------------------------------------------------*/
CU_CBasicRunMode CU_cbasic_get_mode(void)
{
  return f_run_mode;
}

/*------------------------------------------------------------------------*/
void CU_cbasic_show_failures(CU_pFailureRecord pFailure)
{
  int i;

  for (i = 1 ; (NULL != pFailure) ; pFailure = pFailure->pNext, i++) {
    fprintf(stdout, "\n  %d. %s:%u  - %s", i,
        (NULL != pFailure->strFileName) ? pFailure->strFileName : "",
        pFailure->uiLineNumber,
        (NULL != pFailure->strCondition) ? pFailure->strCondition : "");
  }
}

/*=================================================================
 *  Static module functions
 *=================================================================*/
/** Performs inialization actions for the basic interface.
 *  This includes setting output to unbuffered, printing a
 *  welcome message, and setting the test run handlers.
 *  @return An error code indicating the framework error condition.
 */
static CU_ErrorCode cbasic_initialize(void)
{
  /* Unbuffered output so everything reaches the screen */
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);

#ifdef CBASIC_NO_ANSI
  hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
#endif

  CU_set_error(CUE_SUCCESS);

  if (CU_CBRM_SILENT != f_run_mode)
    fprintf(stdout, "\n\n     %s" CU_VERSION
                      "\n     %s\n\n",
                    _("CUnit - A unit testing framework for C - Version "),
                    _("http://cunit.sourceforge.net/"));

  CU_set_test_start_handler(cbasic_test_start_message_handler);
  CU_set_test_complete_handler(cbasic_test_complete_message_handler);
  CU_set_all_test_complete_handler(cbasic_all_tests_complete_message_handler);
  CU_set_suite_init_failure_handler(cbasic_suite_init_failure_message_handler);
  CU_set_suite_cleanup_failure_handler(cbasic_suite_cleanup_failure_message_handler);

  return CU_get_error();
}

/*------------------------------------------------------------------------*/
/** Runs all tests within the basic interface.
 *  If non-NULL, the test registry is changed to the specified registry
 *  before running the tests, and reset to the original registry when
 *  done.  If NULL, the default CUnit test registry will be used.
 *  @param pRegistry The CU_pTestRegistry containing the tests
 *                   to be run.  If NULL, use the default registry.
 *  @return An error code indicating the error status
 *          during the test run.
 */
static CU_ErrorCode cbasic_run_all_tests(CU_pTestRegistry pRegistry)
{
  CU_pTestRegistry pOldRegistry = NULL;
  CU_ErrorCode result;

  f_pRunningSuite = NULL;

  if (NULL != pRegistry)
    pOldRegistry = CU_set_registry(pRegistry);
  result = CU_run_all_tests();
  if (NULL != pRegistry)
    CU_set_registry(pOldRegistry);
  return result;
}

/*------------------------------------------------------------------------*/
/** Runs a specified suite within the basic interface.
 *  @param pSuite The suite to be run (non-NULL).
 *  @return An error code indicating the error status
 *          during the test run.
 */
static CU_ErrorCode cbasic_run_suite(CU_pSuite pSuite)
{
  f_pRunningSuite = NULL;
  return CU_run_suite(pSuite);
}

/*------------------------------------------------------------------------*/
/** Runs a single test for the specified suite within
 *  the console interface.
 *  @param pSuite The suite containing the test to be run (non-NULL).
 *  @param pTest  The test to be run (non-NULL).
 *  @return An error code indicating the error status
 *          during the test run.
 */
static CU_ErrorCode cbasic_run_single_test(CU_pSuite pSuite, CU_pTest pTest)
{
  f_pRunningSuite = NULL;
  return CU_run_test(pSuite, pTest);
}

/*------------------------------------------------------------------------*/
/** Handler function called at start of each test.
 *  @param pTest  The test being run.
 *  @param pSuite The suite containing the test.
 */
static void cbasic_test_start_message_handler(const CU_pTest pTest, const CU_pSuite pSuite)
{
  assert(NULL != pSuite);
  assert(NULL != pTest);

  if (CU_CBRM_VERBOSE == f_run_mode) {
    assert(NULL != pTest->pName);
    if ((NULL == f_pRunningSuite) || (f_pRunningSuite != pSuite)) {
      assert(NULL != pSuite->pName);
      fprintf(stdout, "\n%s: %s", _("Suite"), pSuite->pName);
      fprintf(stdout, "\n  %s: %s ...", _("Test"), pTest->pName);
      f_pRunningSuite = pSuite;
    }
    else {
      fprintf(stdout, "\n  %s: %s ...", _("Test"), pTest->pName);
    }
  }
}

/*------------------------------------------------------------------------*/
/** Handler function called at completion of each test.
 *  @param pTest   The test being run.
 *  @param pSuite  The suite containing the test.
 *  @param pFailure Pointer to the 1st failure record for this test.
 */
static void cbasic_test_complete_message_handler(const CU_pTest pTest,
                                                const CU_pSuite pSuite,
                                                const CU_pFailureRecord pFailureList)
{
  CU_pFailureRecord pFailure = pFailureList;
  int i;

  assert(NULL != pSuite);
  assert(NULL != pTest);

  if (NULL == pFailure) {
    if (CU_CBRM_VERBOSE == f_run_mode) {
      CU_CBASIC_SET_COLOUR(CU_CBRM_PASS_COLOUR);
      fprintf(stdout, _("passed"));
      CU_CBASIC_SET_COLOUR(CU_CBRM_NORMAL_COLOUR);
    }
  }
  else {
    switch (f_run_mode) {
      case CU_CBRM_VERBOSE:
        if (CUF_TestInactive == pFailure->type)
        {
          CU_CBASIC_SET_COLOUR(CU_CBRM_SKIP_COLOUR);
          fprintf(stdout, _("SKIPPED"));
          CU_CBASIC_SET_COLOUR(CU_CBRM_NORMAL_COLOUR);
        }
        else
        {
          CU_CBASIC_SET_COLOUR(CU_CBRM_FAIL_COLOUR);
          fprintf(stdout, _("FAILED"));
          CU_CBASIC_SET_COLOUR(CU_CBRM_NORMAL_COLOUR);
        }
        break;
      case CU_CBRM_NORMAL:
        assert(NULL != pSuite->pName);
        assert(NULL != pTest->pName);
        fprintf(stdout, _("\nSuite %s, Test %s had failures:"), pSuite->pName, pTest->pName);
        break;
      default:  /* gcc wants all enums covered.  ok. */
        break;
    }
    if (CU_CBRM_SILENT != f_run_mode) {
      for (i = 1 ; (NULL != pFailure) ; pFailure = pFailure->pNext, i++) {
        fprintf(stdout, "\n    %d. %s:%u  - %s", i,
            (NULL != pFailure->strFileName) ? pFailure->strFileName : "",
            pFailure->uiLineNumber,
            (NULL != pFailure->strCondition) ? pFailure->strCondition : "");
      }
    }
  }
}

/*------------------------------------------------------------------------*/
/** Handler function called at completion of all tests in a suite.
 *  @param pFailure Pointer to the test failure record list.
 */
static void cbasic_all_tests_complete_message_handler(const CU_pFailureRecord pFailure)
{
  CU_UNREFERENCED_PARAMETER(pFailure); /* not used in basic interface */
  printf("\n\n");
  CU_print_run_results(stdout);
  printf("\n");
  fprintf(stdout, "\n Overall Result: ");
  if ((pFailure != NULL))
  {
    CU_CBASIC_SET_COLOUR(CU_CBRM_FAIL_COLOUR);
    fprintf(stdout, "FAILED ");
    CU_CBASIC_SET_COLOUR(CU_CBRM_NORMAL_COLOUR);
    fprintf(stdout, "(some tests failed, were skipped or there are problems with suites inits/cleanups)\n");
  }
  else
  {
    CU_CBASIC_SET_COLOUR(CU_CBRM_PASS_COLOUR);
    fprintf(stdout, "PASSED\n");
    CU_CBASIC_SET_COLOUR(CU_CBRM_NORMAL_COLOUR);
  }
}

/*------------------------------------------------------------------------*/
/** Handler function called when suite initialization fails.
 *  @param pSuite The suite for which initialization failed.
 */
static void cbasic_suite_init_failure_message_handler(const CU_pSuite pSuite)
{
  assert(NULL != pSuite);
  assert(NULL != pSuite->pName);

  if (CU_CBRM_SILENT != f_run_mode)
  {
    CU_CBASIC_SET_COLOUR(CU_CBRM_SKIP_COLOUR);
    fprintf(stdout, _("\nWARNING"));
    CU_CBASIC_SET_COLOUR(CU_CBRM_NORMAL_COLOUR);
    fprintf(stdout, _(" - Suite initialization failed for '%s'."), pSuite->pName);
  }
}

/*------------------------------------------------------------------------*/
/** Handler function called when suite cleanup fails.
 *  @param pSuite The suite for which cleanup failed.
 */
static void cbasic_suite_cleanup_failure_message_handler(const CU_pSuite pSuite)
{
  assert(NULL != pSuite);
  assert(NULL != pSuite->pName);

  if (CU_CBRM_SILENT != f_run_mode)
  {
    CU_CBASIC_SET_COLOUR(CU_CBRM_SKIP_COLOUR);
    fprintf(stdout, _("\nWARNING"));
    CU_CBASIC_SET_COLOUR(CU_CBRM_NORMAL_COLOUR);
    fprintf(stdout, _(" - Suite cleanup failed for '%s'."), pSuite->pName);
  }
}

/** @} */
