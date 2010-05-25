/* testmultiplexer.cpp - Copyright (C) 2010 Red Hat, Inc.
 * Written by Darryl L. Pierce <dpierce@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.  A copy of the GNU General Public License is
 * also available at http://www.gnu.org/copyleft/gpl.html.
 */

#include "testmultiplexer.h"

#include "virtio/multiplexer.h"

#include <cppunit/TestAssert.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestSuite.h>
#include <cppunit/ui/text/TestRunner.h>

const string APINAME("testapifunction");

const char*
test_callback_function(const char* input)
{
  return input;
}

TestMultiplexer::TestMultiplexer()
  :TestCase("TestMultiplexer")
{}

TestSuite*
TestMultiplexer::suite()
{
  CppUnit::TestSuite *result = new CppUnit::TestSuite("TestMultiplexer");

  result->addTest(new CppUnit::TestCaller<TestMultiplexer>
		  ("testBadAPINameGetsNull",
		   &TestMultiplexer::testUnregisteredAPIIsNull));
  result->addTest(new CppUnit::TestCaller<TestMultiplexer>
		  ("testRegisterAPIMethod",
		   &TestMultiplexer::testRegisterAPIMethod));
  result->addTest(new CppUnit::TestCaller<TestMultiplexer>
		  ("testEnsureCallingAnUnregisteredAPIFails",
		   &TestMultiplexer::testEnsureCallingAnUnregisteredAPIFails));
  result->addTest(new CppUnit::TestCaller<TestMultiplexer>
		  ("testEnsureAPIIsCalled",
		   &TestMultiplexer::testEnsureAPIIsCalled));

  return result;
}

void
TestMultiplexer::setUp()
{
  Multiplexer::instance()->registerAPI(APINAME, test_callback_function);
}

void
TestMultiplexer::testUnregisteredAPIIsNull()
{
  CPPUNIT_ASSERT(NULL == Multiplexer::instance()->getAPI(APINAME + "!"));
}

void
TestMultiplexer::testRegisterAPIMethod()
{
  t_apifunction apifunction = test_callback_function;

  CPPUNIT_ASSERT(apifunction == Multiplexer::instance()->getAPI(APINAME));
}

void
TestMultiplexer::testEnsureCallingAnUnregisteredAPIFails()
{
  string input  = string("This is some input");
  string result = Multiplexer::instance()->invokeAPI(APINAME + "!", input);

  CPPUNIT_ASSERT(string("") == result);
}

void TestMultiplexer::testEnsureAPIIsCalled()
{
  string expected = "This is what I expected";
  string result   = Multiplexer::instance()->invokeAPI(APINAME, expected);

  CPPUNIT_ASSERT(expected == result);
}

int
main(int argc, char** argv)
{
  CppUnit::TextUi::TestRunner runner;

  runner.addTest(TestMultiplexer::suite());
  runner.run();

  return 0;
}
