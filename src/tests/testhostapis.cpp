/* testhostapis.h - Copyright (C) 2010 Red Hat, Inc.
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

#include "testhostapis.h"

#include "host.h"
#include <cppunit/TestCaller.h>
#include <cppunit/ui/text/TestRunner.h>

TestHostApis::TestHostApis()
  :TestCase("TestHostApis")
{}

TestHostApis::~TestHostApis()
{}

TestSuite*
TestHostApis::suite()
{
  TestSuite* result = new TestSuite("Host APIs");

  result->addTest(new TestCaller<TestHostApis>
		  ("testHostGetUUID",
		   &TestHostApis::testHostGetUUID));
  result->addTest(new TestCaller<TestHostApis>
		  ("testHostGetHostname",
		   &TestHostApis::testHostGetHostname));
  result->addTest(new TestCaller<TestHostApis>
		  ("testHostGetHypervisor",
		   &TestHostApis::testHostGetHypervisor));
  result->addTest(new TestCaller<TestHostApis>
		  ("testHostGetArchitecture",
		   &TestHostApis::testHostGetArchitecture));
  result->addTest(new TestCaller<TestHostApis>
		  ("testHostGetMemory",
		   &TestHostApis::testHostGetMemory));
  result->addTest(new TestCaller<TestHostApis>
		  ("testHostGetCpuModel",
		   &TestHostApis::testHostGetCpuModel));
  result->addTest(new TestCaller<TestHostApis>
		  ("testHostGetNumberOfCpuCores",
		   &TestHostApis::testHostGetNumberOfCpuCores));
  result->addTest(new TestCaller<TestHostApis>
		  ("testHostGetLoadAverage",
		   &TestHostApis::testHostGetLoadAverage));

  return result;
}

void
TestHostApis::setUp()
{
  this->platform = Platform::instance();
}

void
TestHostApis::testHostGetUUID()
{
  CPPUNIT_ASSERT(platform->getUUID() == host_get_uuid());
}

void
TestHostApis::testHostGetHostname()
{
  CPPUNIT_ASSERT(platform->getHostname() == host_get_hostname());
}

void
TestHostApis::testHostGetHypervisor()
{
  CPPUNIT_ASSERT(platform->getHypervisor() == host_get_hypervisor());
}

void
TestHostApis::testHostGetArchitecture()
{
  CPPUNIT_ASSERT(platform->getArchitecture() == host_get_architecture());
}

void
TestHostApis::testHostGetMemory()
{
  CPPUNIT_ASSERT(platform->getMemory() == host_get_memory());
}

void
TestHostApis::testHostGetCpuModel()
{
  CPPUNIT_ASSERT(platform->getCPUModel() == host_get_cpu_model());
}

void
TestHostApis::testHostGetNumberOfCpuCores()
{
  CPPUNIT_ASSERT(platform->getNumberOfCPUCores() == host_get_number_of_cpu_cores());
}

void
TestHostApis::testHostGetLoadAverage()
{
  CPPUNIT_ASSERT(platform->getLoadAverage() == host_get_load_average());
}

int
main(int argc, char** argv)
{
  TextUi::TestRunner runner;

  runner.addTest(TestHostApis::suite());
  runner.run();

  return 0;
}
