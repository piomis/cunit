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
  *  Implementation of the JUnit Report Format
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
#include "Report_JUnit.h"

#define MAX_FILENAME_LENGTH   1025

/*=================================================================
*  Global / Static data definitions
*=================================================================*/

CU_reportFormat_T CU_reportFormat_JUnit =
{
  CU_report_JUnit_set_output_filename,                  /* pSetOutputFilename */
  CU_report_JUnit_open_report,                          /* pOpenReport */
  CU_report_JUnit_close_report,                         /* pCloseReport */
  NULL,                                                 /* pTestStartMsgHandler */
  NULL,                                                 /* pTestCompleteMsgHandler */
  CU_report_JUnit_all_tests_complete_msg_handler,       /* pAllTestsCompleteMsgHandler */
  NULL,                                                 /* pSuiteInitFailureMsgHandler */
  NULL,                                                 /* pSuiteCleanupFailureMsgHandler */
  CU_report_JUnit_suite_complete_msg_handler,           /* pSuiteCompleteMsgHandler */
  NULL                                                  /* pListAllTests - not supported by JUnit format */
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

static void CU_report_JUnit_print_single_test_success(const CU_pTest pTest);
static void CU_report_JUnit_print_single_test_error(const CU_pTest pTest);
static void CU_report_JUnit_print_single_test_skipped(const CU_pTest pTest);
static CU_pFailureRecord CU_report_JUnit_print_single_test_failed(const CU_pTest pTest, const CU_pFailureRecord pFailure);
static void CU_report_JUnit_print_testcase_tag(const CU_pTest pTest, const CU_BOOL hasSubTags);
static void CU_report_JUnit_print_dummy_test(const char* sSuiteName, const CU_pFailureRecord pFailure);
static void CU_report_JUnit_print_failure_details(CU_pFailureRecord pFailure);
static void CU_report_JUnit_get_failure_msg(char* strCondition, char ** pOutputBuffer);

/*=================================================================
*  Public Interface functions
*=================================================================*/

void CU_report_JUnit_set_output_filename(const char* szFilename)
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

CU_ErrorCode CU_report_JUnit_open_report(void)
{
  CU_pRunSummary pRunSummary = CU_get_run_summary();

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
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<testsuites errors=\"0\" failures=\"%d\" tests=\"%d\" name=\"\"> \n",
      pRunSummary->nTestsFailed,
      pRunSummary->nTestsRun);
  }

  return CU_get_error();
}

/*------------------------------------------------------------------------*/

CU_ErrorCode CU_report_JUnit_close_report(void)
{
  assert(NULL != f_pTestResultFile);

  CU_set_error(CUE_SUCCESS);

  if (0 != fclose(f_pTestResultFile)) {
    CU_set_error(CUE_FCLOSE_FAILED);
  }

  return CU_get_error();
}

/*------------------------------------------------------------------------*/
/** Handler function called at completion of all tests.
 *  @param pFailure Pointer to the test failure record list.
 */
void CU_report_JUnit_all_tests_complete_msg_handler(const CU_pFailureRecord pFailure)
{
  CU_pTestRegistry pRegistry = CU_get_registry();
  CU_pRunSummary pRunSummary = CU_get_run_summary();

  CU_UNREFERENCED_PARAMETER(pFailure);  /* not used */

  assert(NULL != pRegistry);
  assert(NULL != pRunSummary);
  assert(NULL != f_pTestResultFile);

  fprintf(f_pTestResultFile, "</testsuites>");
}

/*------------------------------------------------------------------------*/
/** Handler function called when suite is completed.
 *  @param pSuite The suite which has completed
 *  @param pFailure Pointer to the test failure record list.
 */
void CU_report_JUnit_suite_complete_msg_handler(const CU_pSuite pSuite, const CU_pFailureRecord pFailure)
{
  char *szTempName = NULL;
  size_t szTempName_len = 0;
  CU_pTest pTest;
  CU_pFailureRecord pCurrFailure;

  assert(NULL != pSuite);
  assert(NULL != pSuite->pName);
  assert(NULL != f_pTestResultFile);

  /* translate suite name that may contain XML control characters */
  szTempName = (char *)CU_MALLOC((szTempName_len = CU_translated_strlen(pSuite->pName) + 1));
  CU_translate_special_characters(pSuite->pName, szTempName, szTempName_len);

  /* Print suite open tag */
  fprintf(f_pTestResultFile,
    /*"  <testsuite errors=\"%d\" failures=\"%d\" tests=\"%d\" name=\"%s\"> \n",*/
    "  <testsuite tests=\"%d\" name=\"%s\"> \n",
    //0, /* Errors */
    //pSuite->uiNumberOfTestsFailed, /* Failures */
    pSuite->uiNumberOfTests, /* Tests */
    (NULL != szTempName) ? szTempName : ""); /* Name */

  if (pFailure != NULL)
  {
    pCurrFailure = pFailure;

    /* Check if first failure is a Suite Init failure */
    if (CUF_SuiteInitFailed == pCurrFailure->type)
    {
      /* Add failed dummy test case */
      CU_report_JUnit_print_dummy_test(szTempName, pCurrFailure);

      pCurrFailure = pCurrFailure->pNext;

      /* Print all tests in suite as "errored" */
      pTest = pSuite->pTest;
      while (pTest != NULL)
      {
        CU_report_JUnit_print_single_test_error(pTest);
        pTest = pTest->pNext;
      }
    }
    else /**/
    {
      pTest = pSuite->pTest;
      while (pTest != NULL)
      {
        /* Check if there are any failure records for given test. */
        if ((pCurrFailure != NULL) && (pCurrFailure->pTest == pTest))
        {
          if (CUF_TestInactive == pCurrFailure->type)
          {
            CU_report_JUnit_print_single_test_skipped(pTest);
          }
          else
          {
            pCurrFailure = CU_report_JUnit_print_single_test_failed(pTest, pCurrFailure);
          }
        }
        else
        {
          CU_report_JUnit_print_single_test_success(pTest);
        }
        pTest = pTest->pNext;
      }

      if ((pCurrFailure != NULL) && (CUF_SuiteCleanupFailed == pCurrFailure->type))
      {
        /* Add failed dummy test case */
        CU_report_JUnit_print_dummy_test(szTempName, pCurrFailure);
      }
    }
  }
  else /* No failure record - all tests & init/cleanup of suite passed */
  {
    pTest = pSuite->pTest;
    while (pTest != NULL)
    {
      CU_report_JUnit_print_single_test_success(pTest);
      pTest = pTest->pNext;
    }
  }

  /* Print suite close tag. */
  fprintf(f_pTestResultFile,
    "  </testsuite>\n");

  if (NULL != szTempName) {
    CU_FREE(szTempName);
  }
}

/*------------------------------------------------------------------------*/
/** Function prints single success test entry in report
 *  @param pTest Test to print
 */
static void CU_report_JUnit_print_single_test_success(const CU_pTest pTest)
{
  CU_report_JUnit_print_testcase_tag(pTest, CU_FALSE);
}

/*------------------------------------------------------------------------*/
/** Function prints single error test entry in report
 *  @param pTest Test to print
 */
static void CU_report_JUnit_print_single_test_error(const CU_pTest pTest)
{
  CU_report_JUnit_print_testcase_tag(pTest, CU_TRUE);

  fprintf(f_pTestResultFile, "      <error message=\"Suite initialization failed\"/>\n");

  fprintf(f_pTestResultFile, "    </testcase>\n");
}

/*------------------------------------------------------------------------*/
/** Function prints single skipped test entry in report
 *  @param pTest Test to print
 */
static void CU_report_JUnit_print_single_test_skipped(const CU_pTest pTest)
{
  CU_report_JUnit_print_testcase_tag(pTest, CU_TRUE);

  fprintf(f_pTestResultFile, "      <skipped/>\n");

  fprintf(f_pTestResultFile, "    </testcase>\n");
}

/*------------------------------------------------------------------------*/
/** Function prints single failed test entry in report
 *  @param pTest Test to print
 *  @param pFailure First failure of test
 *  @returns CU_pFailureRecord failure entry of next test or null
 */
static CU_pFailureRecord CU_report_JUnit_print_single_test_failed(const CU_pTest pTest, const CU_pFailureRecord pFailure)
{
  char * szTemp = NULL;

  CU_pFailureRecord pTempFailure = pFailure;

  CU_report_JUnit_print_testcase_tag(pTest, CU_TRUE);

  CU_report_JUnit_get_failure_msg(pFailure->strCondition, &szTemp);
  fprintf(f_pTestResultFile, "      <failure message=\"%s\" type=\"Failure\">\n", szTemp);
  if (NULL != szTemp)
  {
    CU_FREE(szTemp);
  }

  while (NULL != pTempFailure && (pTempFailure->pTest == pTest))
  {
    CU_report_JUnit_print_failure_details(pTempFailure);

    pTempFailure = pTempFailure->pNext;
  } /* while */

  fprintf(f_pTestResultFile, "      </failure>\n");

  fprintf(f_pTestResultFile, "    </testcase>\n");

  return pTempFailure;
}

/*------------------------------------------------------------------------*/
/** Function prints single test tag
 *  @param pTest Test to print
 *  @param hasSubTags Flag to indicate if test case tag will contain sub-tags (like <error> or <failure>)
 */
static void CU_report_JUnit_print_testcase_tag(const CU_pTest pTest, const CU_BOOL hasSubTags)
{
  const char *pPackageName = CU_automated_package_name_get();

  /* Test tag */
  fprintf(f_pTestResultFile, "    <testcase classname=\"%s.%s\" name=\"%s\" time=\"0\"%s>\n",
    pPackageName,
    "",
    (NULL != pTest->pName) ? pTest->pName : "",
    (CU_TRUE == hasSubTags) ? "" : "/");
  }

/*------------------------------------------------------------------------*/
/** Function prints dummy test tag for failed test suite init/cleanup
 *  @param pSuite Suite for which initialization/cleanup failed
 *  @param pFailure Failure record
 */
static void CU_report_JUnit_print_dummy_test(const char* sSuiteName, const CU_pFailureRecord pFailure)
{
  const char *pPackageName = CU_automated_package_name_get();

  assert(NULL != sSuiteName);
  assert(NULL != pFailure);

  if (CUF_SuiteInitFailed == pFailure->type)
  {
    fprintf(f_pTestResultFile, "    <testcase classname=\"%s.%s\" name=\"%s - Initialization\" time=\"0\">\n"
        "      <failure message=\"Suite Initialization failed\" type=\"Failure\">\n",
      pPackageName,
      "",
      sSuiteName);
  }
  else
  {
    fprintf(f_pTestResultFile, "    <testcase classname=\"%s.%s\" name=\"%s - Cleanup\" time=\"0\">\n"
        "      <failure message=\"Suite Cleanup failed\" type=\"Failure\">\n",
      pPackageName,
      "",
      sSuiteName);
  }

  CU_report_JUnit_print_failure_details(pFailure);

  fprintf(f_pTestResultFile, "      </failure>\n"
    "    </testcase>\n");
}

/*------------------------------------------------------------------------*/
/** Function printf formated failure details
 *  @param pFailure Failure record
 */
static void CU_report_JUnit_print_failure_details(CU_pFailureRecord pFailure)
{
  char * szTemp = NULL;

  CU_report_JUnit_get_failure_msg(pFailure->strCondition, &szTemp);

  fprintf(f_pTestResultFile, "        Condition: %s\n", szTemp);
  fprintf(f_pTestResultFile, "        File     : %s\n", (NULL != pFailure->strFileName) ? pFailure->strFileName : "");
  fprintf(f_pTestResultFile, "        Line     : %d\n", pFailure->uiLineNumber);

  if (NULL != szTemp)
  {
    CU_FREE(szTemp);
  }
}

static void CU_report_JUnit_get_failure_msg(char* strCondition, char ** pOutputBuffer)
{
  size_t cur_len;

  /* expand temporary char buffer if need more room */
  if (NULL != strCondition) {
    cur_len = CU_translated_strlen(strCondition) + 1;
  }
  else {
    cur_len = 1;
  }

  if (NULL != *pOutputBuffer)
  {
    CU_FREE(*pOutputBuffer);
  }
  *pOutputBuffer = (char *)CU_MALLOC(cur_len);

  /* convert xml entities in strCondition (if present) */
  if (NULL != strCondition) {
    CU_translate_special_characters(strCondition, *pOutputBuffer, cur_len);
  }
  else {
    (*pOutputBuffer)[0] = '\0';
  }
}
   /** @} */
