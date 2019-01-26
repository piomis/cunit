/*
 *  CUnit - A Unit testing framework library for C.
 *  Copyright (C) 2001       Anil Kumar
 *  Copyright (C) 2004-2006  Anil Kumar, Jerry St.Clair
 *  Copyright (C) 2019       Piotr Mis
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
  *  Implementation of the CUnit Report Format
  *
  *  24-Jan-2019      Initial implementation. (PMi)
  *
  */

  /** @file
   * Automated test interface with xml result output (implementation).
   */
   /** @addtogroup Automated
    @{
   */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#include "CUnit.h"
#include "CUnit_intl.h"
#include "MyMem.h"
#include "Util.h"
#include "Report_CUnit.h"

#define MAX_FILENAME_LENGTH   1025

/*=================================================================
*  Global / Static data definitions
*=================================================================*/

CU_reportFormat_T CU_reportFormat_CUnit =
{
  CU_report_CUnit_set_output_filename,                  /* pSetOutputFilename */
  CU_report_CUnit_open_report,                          /* pOpenReport */
  CU_report_CUnit_close_report,                         /* pCloseReport */
  CU_report_CUnit_test_start_msg_handler,               /* pTestStartMsgHandler */
  CU_report_CUnit_test_complete_msg_handler,            /* pTestCompleteMsgHandler */
  CU_report_CUnit_all_tests_complete_msg_handler,       /* pAllTestsCompleteMsgHandler */
  CU_report_CUnit_suite_init_failure_msg_handler,       /* pSuiteInitFailureMsgHandler */
  CU_report_CUnit_suite_cleanup_failure_msg_handler,    /* pSuiteCleanupFailureMsgHandler */
  NULL,                                                 /* pSuiteCompleteMsgHandler */
  CU_report_CUnit_list_all_tests                        /* pListAllTests */
};

/*=================================================================
 *  Static function forward declarations
 *=================================================================*/
static CU_pSuite f_pRunningSuite = NULL;                    /**< The running test suite. */
static char      f_szDefaultFileRoot[] = "CUnitAutomated";  /**< Default filename root for automated output files. */
static char      f_szTestListFileName[MAX_FILENAME_LENGTH] = "";   /**< Current output file name for the test listing file. */
static char      f_szTestResultFileName[MAX_FILENAME_LENGTH] = ""; /**< Current output file name for the test results file. */
static FILE*     f_pTestResultFile = NULL;                  /**< FILE pointer the test results file. */

static CU_BOOL f_bWriting_CUNIT_RUN_SUITE = CU_FALSE;       /**< Flag for keeping track of when a closing xml tag is required. */

/*=================================================================
*  Public Interface functions
*=================================================================*/

void CU_report_CUnit_set_output_filename(const char* szFilename)
{
  const char* szListEnding = "-Listing.xml";
  const char* szResultEnding = "-Results.xml";

  /* Construct the name for the listing file */
  if (NULL != szFilename) {
    strncpy(f_szTestListFileName, szFilename, MAX_FILENAME_LENGTH - strlen(szListEnding) - 1);
  }
  else {
    strncpy(f_szTestListFileName, f_szDefaultFileRoot, MAX_FILENAME_LENGTH - strlen(szListEnding) - 1);
  }

  f_szTestListFileName[MAX_FILENAME_LENGTH - strlen(szListEnding) - 1] = '\0';
  strcat(f_szTestListFileName, szListEnding);

  /* Construct the name for the result file */
  if (NULL != szFilename) {
    strncpy(f_szTestResultFileName, szFilename, MAX_FILENAME_LENGTH - strlen(szResultEnding) - 1);
  }
  else {
    strncpy(f_szTestResultFileName, f_szDefaultFileRoot, MAX_FILENAME_LENGTH - strlen(szResultEnding) - 1);
  }

  f_szTestResultFileName[MAX_FILENAME_LENGTH - strlen(szResultEnding) - 1] = '\0';
  strcat(f_szTestResultFileName, szResultEnding);
}

/*------------------------------------------------------------------------*/

CU_ErrorCode CU_report_CUnit_open_report(void)
{
  /* if a filename root hasn't been set, use the default one */
  if (0 == strlen(f_szTestResultFileName)) {
    CU_set_output_filename(f_szDefaultFileRoot);
  }

  f_bWriting_CUNIT_RUN_SUITE = CU_FALSE;

  f_pRunningSuite = NULL;

  CU_set_error(CUE_SUCCESS);

  if (NULL == (f_pTestResultFile = fopen(f_szTestResultFileName, "w"))) {
    CU_set_error(CUE_FOPEN_FAILED);
  }
  else {
    setvbuf(f_pTestResultFile, NULL, _IONBF, 0);

    fprintf(f_pTestResultFile,
      "<?xml version=\"1.0\" ?> \n"
      "<?xml-stylesheet type=\"text/xsl\" href=\"CUnit-Run.xsl\" ?> \n"
      "<!DOCTYPE CUNIT_TEST_RUN_REPORT SYSTEM \"CUnit-Run.dtd\"> \n"
      "<CUNIT_TEST_RUN_REPORT> \n"
      "  <CUNIT_HEADER/> \n");
    fprintf(f_pTestResultFile, "  <CUNIT_RESULT_LISTING> \n");
  }

  return CU_get_error();
}

/*------------------------------------------------------------------------*/

CU_ErrorCode CU_report_CUnit_close_report(void)
{
  char* szTime;
  time_t tTime = 0;

  assert(NULL != f_pTestResultFile);

  CU_set_error(CUE_SUCCESS);

  time(&tTime);
  szTime = ctime(&tTime);
  fprintf(f_pTestResultFile,
    "  <CUNIT_FOOTER> %s" CU_VERSION " - %s </CUNIT_FOOTER> \n"
    "</CUNIT_TEST_RUN_REPORT>",
    _("File Generated By CUnit v"),
    (NULL != szTime) ? szTime : "");

  if (0 != fclose(f_pTestResultFile)) {
    CU_set_error(CUE_FCLOSE_FAILED);
  }

  return CU_get_error();
}

/*------------------------------------------------------------------------*/
/** Handler function called at start of each test.
 *  The test result file must have been opened before this
 *  function is called (i.e. f_pTestResultFile non-NULL).
 *  @param pTest  The test being run (non-NULL).
 *  @param pSuite The suite containing the test (non-NULL).
 */
void CU_report_CUnit_test_start_msg_handler(const CU_pTest pTest, const CU_pSuite pSuite)
{
  char *szTempName = NULL;
  size_t szTempName_len = 0;

  CU_UNREFERENCED_PARAMETER(pTest);   /* not currently used */

  assert(NULL != pTest);
  assert(NULL != pSuite);
  assert(NULL != pSuite->pName);
  assert(NULL != f_pTestResultFile);

  /* write suite close/open tags if this is the 1st test for this szSuite */
  if ((NULL == f_pRunningSuite) || (f_pRunningSuite != pSuite)) {
    if (CU_TRUE == f_bWriting_CUNIT_RUN_SUITE) {
      fprintf(f_pTestResultFile,
        "      </CUNIT_RUN_SUITE_SUCCESS> \n"
        "    </CUNIT_RUN_SUITE> \n");
    }

    /* translate suite name that may contain XML control characters */
    szTempName = (char *)CU_MALLOC((szTempName_len = CU_translated_strlen(pSuite->pName) + 1));
    CU_translate_special_characters(pSuite->pName, szTempName, szTempName_len);

    fprintf(f_pTestResultFile,
      "    <CUNIT_RUN_SUITE> \n"
      "      <CUNIT_RUN_SUITE_SUCCESS> \n"
      "        <SUITE_NAME> %s </SUITE_NAME> \n",
      (NULL != szTempName ? szTempName : ""));

    f_bWriting_CUNIT_RUN_SUITE = CU_TRUE;
    f_pRunningSuite = pSuite;
  }

  if (NULL != szTempName) {
    CU_FREE(szTempName);
  }
}

/*------------------------------------------------------------------------*/
/** Handler function called at completion of each test.
 * @param pTest   The test being run (non-NULL).
 * @param pSuite  The suite containing the test (non-NULL).
 * @param pFailure Pointer to the 1st failure record for this test.
 */
void CU_report_CUnit_test_complete_msg_handler(const CU_pTest pTest, const CU_pSuite pSuite, const CU_pFailureRecord pFailure)
{
  char *szTemp = NULL;
  size_t szTemp_len = 0;
  size_t cur_len = 0;
  CU_pFailureRecord pTempFailure = pFailure;

  CU_UNREFERENCED_PARAMETER(pSuite);  /* pSuite is not used except in assertion */

  assert(NULL != pTest);
  assert(NULL != pTest->pName);
  assert(NULL != pSuite);
  assert(NULL != pSuite->pName);
  assert(NULL != f_pTestResultFile);

  if (NULL != pTempFailure) {

    while (NULL != pTempFailure) {

      assert((NULL != pTempFailure->pSuite) && (pTempFailure->pSuite == pSuite));
      assert((NULL != pTempFailure->pTest) && (pTempFailure->pTest == pTest));

      /* expand temporary char buffer if need more room */
      if (NULL != pTempFailure->strCondition) {
        cur_len = CU_translated_strlen(pTempFailure->strCondition) + 1;
      }
      else {
        cur_len = 1;
      }
      if (cur_len > szTemp_len) {
        szTemp_len = cur_len;
        if (NULL != szTemp) {
          CU_FREE(szTemp);
        }
        szTemp = (char *)CU_MALLOC(szTemp_len);
      }

      /* convert xml entities in strCondition (if present) */
      if (NULL != pTempFailure->strCondition) {
        CU_translate_special_characters(pTempFailure->strCondition, szTemp, szTemp_len);
      }
      else {
        szTemp[0] = '\0';
      }

      fprintf(f_pTestResultFile,
        "        <CUNIT_RUN_TEST_RECORD> \n"
        "          <CUNIT_RUN_TEST_FAILURE> \n"
        "            <TEST_NAME> %s </TEST_NAME> \n"
        "            <FILE_NAME> %s </FILE_NAME> \n"
        "            <LINE_NUMBER> %u </LINE_NUMBER> \n"
        "            <CONDITION> %s </CONDITION> \n"
        "          </CUNIT_RUN_TEST_FAILURE> \n"
        "        </CUNIT_RUN_TEST_RECORD> \n",
        pTest->pName,
        (NULL != pTempFailure->strFileName) ? pTempFailure->strFileName : "",
        pTempFailure->uiLineNumber,
        szTemp);

      pTempFailure = pTempFailure->pNext;
    } /* while */
  }
  else {
    fprintf(f_pTestResultFile,
      "        <CUNIT_RUN_TEST_RECORD> \n"
      "          <CUNIT_RUN_TEST_SUCCESS> \n"
      "            <TEST_NAME> %s </TEST_NAME> \n"
      "          </CUNIT_RUN_TEST_SUCCESS> \n"
      "        </CUNIT_RUN_TEST_RECORD> \n",
      pTest->pName);
  }

  if (NULL != szTemp) {
    CU_FREE(szTemp);
  }
}

/*------------------------------------------------------------------------*/
/** Handler function called at completion of all tests.
 *  @param pFailure Pointer to the test failure record list.
 */
void CU_report_CUnit_all_tests_complete_msg_handler(const CU_pFailureRecord pFailure)
{
  CU_pTestRegistry pRegistry = CU_get_registry();
  CU_pRunSummary pRunSummary = CU_get_run_summary();

  CU_UNREFERENCED_PARAMETER(pFailure);  /* not used */

  assert(NULL != pRegistry);
  assert(NULL != pRunSummary);
  assert(NULL != f_pTestResultFile);

  if ((NULL != f_pRunningSuite) && (CU_TRUE == f_bWriting_CUNIT_RUN_SUITE)) {
    fprintf(f_pTestResultFile,
      "      </CUNIT_RUN_SUITE_SUCCESS> \n"
      "    </CUNIT_RUN_SUITE> \n");
  }

  fprintf(f_pTestResultFile,
    "  </CUNIT_RESULT_LISTING>\n"
    "  <CUNIT_RUN_SUMMARY> \n");

  fprintf(f_pTestResultFile,
    "    <CUNIT_RUN_SUMMARY_RECORD> \n"
    "      <TYPE> %s </TYPE> \n"
    "      <TOTAL> %u </TOTAL> \n"
    "      <RUN> %u </RUN> \n"
    "      <SUCCEEDED> - NA - </SUCCEEDED> \n"
    "      <FAILED> %u </FAILED> \n"
    "      <INACTIVE> %u </INACTIVE> \n"
    "    </CUNIT_RUN_SUMMARY_RECORD> \n",
    _("Suites"),
    pRegistry->uiNumberOfSuites,
    pRunSummary->nSuitesRun,
    pRunSummary->nSuitesFailed,
    pRunSummary->nSuitesInactive);

  fprintf(f_pTestResultFile,
    "    <CUNIT_RUN_SUMMARY_RECORD> \n"
    "      <TYPE> %s </TYPE> \n"
    "      <TOTAL> %u </TOTAL> \n"
    "      <RUN> %u </RUN> \n"
    "      <SUCCEEDED> %u </SUCCEEDED> \n"
    "      <FAILED> %u </FAILED> \n"
    "      <INACTIVE> %u </INACTIVE> \n"
    "    </CUNIT_RUN_SUMMARY_RECORD> \n",
    _("Test Cases"),
    pRegistry->uiNumberOfTests,
    pRunSummary->nTestsRun,
    pRunSummary->nTestsRun - pRunSummary->nTestsFailed,
    pRunSummary->nTestsFailed,
    pRunSummary->nTestsInactive);

  fprintf(f_pTestResultFile,
    "    <CUNIT_RUN_SUMMARY_RECORD> \n"
    "      <TYPE> %s </TYPE> \n"
    "      <TOTAL> %u </TOTAL> \n"
    "      <RUN> %u </RUN> \n"
    "      <SUCCEEDED> %u </SUCCEEDED> \n"
    "      <FAILED> %u </FAILED> \n"
    "      <INACTIVE> %s </INACTIVE> \n"
    "    </CUNIT_RUN_SUMMARY_RECORD> \n"
    "  </CUNIT_RUN_SUMMARY> \n",
    _("Assertions"),
    pRunSummary->nAsserts,
    pRunSummary->nAsserts,
    pRunSummary->nAsserts - pRunSummary->nAssertsFailed,
    pRunSummary->nAssertsFailed,
    _("n/a"));
}

/*------------------------------------------------------------------------*/
/** Handler function called when suite initialization fails.
 *  @param pSuite The suite for which initialization failed.
 */
void CU_report_CUnit_suite_init_failure_msg_handler(const CU_pSuite pSuite)
{
  assert(NULL != pSuite);
  assert(NULL != pSuite->pName);
  assert(NULL != f_pTestResultFile);

  if (CU_TRUE == f_bWriting_CUNIT_RUN_SUITE) {
    fprintf(f_pTestResultFile,
      "      </CUNIT_RUN_SUITE_SUCCESS> \n"
      "    </CUNIT_RUN_SUITE> \n");
    f_bWriting_CUNIT_RUN_SUITE = CU_FALSE;
  }

  fprintf(f_pTestResultFile,
    "    <CUNIT_RUN_SUITE> \n"
    "      <CUNIT_RUN_SUITE_FAILURE> \n"
    "        <SUITE_NAME> %s </SUITE_NAME> \n"
    "        <FAILURE_REASON> %s </FAILURE_REASON> \n"
    "      </CUNIT_RUN_SUITE_FAILURE> \n"
    "    </CUNIT_RUN_SUITE>  \n",
    pSuite->pName,
    _("Suite Initialization Failed"));
}

/*------------------------------------------------------------------------*/
/** Handler function called when suite cleanup fails.
 *  @param pSuite The suite for which cleanup failed.
 */
void CU_report_CUnit_suite_cleanup_failure_msg_handler(const CU_pSuite pSuite)
{
  assert(NULL != pSuite);
  assert(NULL != pSuite->pName);
  assert(NULL != f_pTestResultFile);

  if (CU_TRUE == f_bWriting_CUNIT_RUN_SUITE) {
    fprintf(f_pTestResultFile,
      "      </CUNIT_RUN_SUITE_SUCCESS> \n"
      "    </CUNIT_RUN_SUITE> \n");
    f_bWriting_CUNIT_RUN_SUITE = CU_FALSE;
  }

  fprintf(f_pTestResultFile,
    "    <CUNIT_RUN_SUITE> \n"
    "      <CUNIT_RUN_SUITE_FAILURE> \n"
    "        <SUITE_NAME> %s </SUITE_NAME> \n"
    "        <FAILURE_REASON> %s </FAILURE_REASON> \n"
    "      </CUNIT_RUN_SUITE_FAILURE> \n"
    "    </CUNIT_RUN_SUITE>  \n",
    pSuite->pName,
    _("Suite Cleanup Failed"));
}

/*------------------------------------------------------------------------*/
/** Generates an xml listing of all tests in all suites for the
 *  specified test registry.  The output is directed to a file
 *  having the specified name.
 *  @param pRegistry   Test registry for which to generate list (non-NULL).
 *  @return  A CU_ErrorCode indicating the error status.
 */
CU_ErrorCode CU_report_CUnit_list_all_tests(CU_pTestRegistry pRegistry)
{
  CU_pSuite pSuite = NULL;
  CU_pTest  pTest = NULL;
  FILE* pTestListFile = NULL;
  char* szTime;
  time_t tTime = 0;

  CU_set_error(CUE_SUCCESS);

  if (NULL == pRegistry) {
    CU_set_error(CUE_NOREGISTRY);
  }
  else if (NULL == (pTestListFile = fopen(f_szTestListFileName, "w"))) {
    CU_set_error(CUE_FOPEN_FAILED);
  }
  else {
    setvbuf(pTestListFile, NULL, _IONBF, 0);

    fprintf(pTestListFile,
      "<?xml version=\"1.0\" ?> \n"
      "<?xml-stylesheet type=\"text/xsl\" href=\"CUnit-List.xsl\" ?> \n"
      "<!DOCTYPE CUNIT_TEST_LIST_REPORT SYSTEM \"CUnit-List.dtd\"> \n"
      "<CUNIT_TEST_LIST_REPORT> \n"
      "  <CUNIT_HEADER/> \n"
      "  <CUNIT_LIST_TOTAL_SUMMARY> \n");

    fprintf(pTestListFile,
      "    <CUNIT_LIST_TOTAL_SUMMARY_RECORD> \n"
      "      <CUNIT_LIST_TOTAL_SUMMARY_RECORD_TEXT> %s </CUNIT_LIST_TOTAL_SUMMARY_RECORD_TEXT> \n"
      "      <CUNIT_LIST_TOTAL_SUMMARY_RECORD_VALUE> %u </CUNIT_LIST_TOTAL_SUMMARY_RECORD_VALUE> \n"
      "    </CUNIT_LIST_TOTAL_SUMMARY_RECORD> \n",
      _("Total Number of Suites"),
      pRegistry->uiNumberOfSuites);

    fprintf(pTestListFile,
      "    <CUNIT_LIST_TOTAL_SUMMARY_RECORD> \n"
      "      <CUNIT_LIST_TOTAL_SUMMARY_RECORD_TEXT> %s </CUNIT_LIST_TOTAL_SUMMARY_RECORD_TEXT> \n"
      "      <CUNIT_LIST_TOTAL_SUMMARY_RECORD_VALUE> %u </CUNIT_LIST_TOTAL_SUMMARY_RECORD_VALUE> \n"
      "    </CUNIT_LIST_TOTAL_SUMMARY_RECORD> \n"
      "  </CUNIT_LIST_TOTAL_SUMMARY> \n",
      _("Total Number of Test Cases"),
      pRegistry->uiNumberOfTests);

    fprintf(pTestListFile,
      "  <CUNIT_ALL_TEST_LISTING> \n");

    pSuite = pRegistry->pSuite;
    while (NULL != pSuite) {
      assert(NULL != pSuite->pName);
      pTest = pSuite->pTest;

      fprintf(pTestListFile,
        "    <CUNIT_ALL_TEST_LISTING_SUITE> \n"
        "      <CUNIT_ALL_TEST_LISTING_SUITE_DEFINITION> \n"
        "        <SUITE_NAME> %s </SUITE_NAME> \n"
        "        <INITIALIZE_VALUE> %s </INITIALIZE_VALUE> \n"
        "        <CLEANUP_VALUE> %s </CLEANUP_VALUE> \n"
        "        <ACTIVE_VALUE> %s </ACTIVE_VALUE> \n"
        "        <TEST_COUNT_VALUE> %u </TEST_COUNT_VALUE> \n"
        "      </CUNIT_ALL_TEST_LISTING_SUITE_DEFINITION> \n",
        pSuite->pName,
        (NULL != pSuite->pInitializeFunc) ? _("Yes") : _("No"),
        (NULL != pSuite->pCleanupFunc) ? _("Yes") : _("No"),
        (CU_FALSE != pSuite->fActive) ? _("Yes") : _("No"),
        pSuite->uiNumberOfTests);

      fprintf(pTestListFile,
        "      <CUNIT_ALL_TEST_LISTING_SUITE_TESTS> \n");
      while (NULL != pTest) {
        assert(NULL != pTest->pName);
        fprintf(pTestListFile,
          "        <TEST_CASE_DEFINITION> \n"
          "          <TEST_CASE_NAME> %s </TEST_CASE_NAME> \n"
          "          <TEST_ACTIVE_VALUE> %s </TEST_ACTIVE_VALUE> \n"
          "        </TEST_CASE_DEFINITION> \n",
          pTest->pName,
          (CU_FALSE != pSuite->fActive) ? _("Yes") : _("No"));
        pTest = pTest->pNext;
      }

      fprintf(pTestListFile,
        "      </CUNIT_ALL_TEST_LISTING_SUITE_TESTS> \n"
        "    </CUNIT_ALL_TEST_LISTING_SUITE> \n");

      pSuite = pSuite->pNext;
    }

    fprintf(pTestListFile, "  </CUNIT_ALL_TEST_LISTING> \n");

    time(&tTime);
    szTime = ctime(&tTime);
    fprintf(pTestListFile,
      "  <CUNIT_FOOTER> %s" CU_VERSION " - %s </CUNIT_FOOTER> \n"
      "</CUNIT_TEST_LIST_REPORT>",
      _("File Generated By CUnit v"),
      (NULL != szTime) ? szTime : "");

    if (0 != fclose(pTestListFile)) {
      CU_set_error(CUE_FCLOSE_FAILED);
    }
  }

  return CU_get_error();
}
   /** @} */
