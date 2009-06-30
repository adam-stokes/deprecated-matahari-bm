/* Copyright (C) 2009 Red Hat, Inc.
 * Written by Arjun Roy <arroy@redhat.com>
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

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>

#include "cpu.h"
#include "qmf/com/redhat/matahari/CPU.h"

using namespace std;
namespace _qmf = qmf::com::redhat::matahari;

ostream& operator<<(ostream& output, const CPUWrapper& cpu) 
{
    output << "Processor" << endl;
    output << "CPU #: " << cpu.cpunum << endl;
    output << "Core #: " << cpu.corenum << endl;
    output << "Num. Cores: " << cpu.numcores << endl;
    output << "Model: " << cpu.model << endl;
    output << "Family: " << cpu.family << endl;
    output << "CPU ID Level: " << cpu.cpuid_lvl << endl;
    output << "Speed (Mhz): " << cpu.speed << endl;
    output << "Cache (kB): " << cpu.cache << endl;
    output << "Vendor: " << cpu.vendor << endl;
    output << "Flags: " << cpu.flags << endl;
    return output;
}

void CPUWrapper::setupQMFObject(ManagementAgent *agent, Manageable *parent)
{
    mgmt_object = new _qmf::CPU(agent, this, parent);
    agent->addObject(mgmt_object);
    syncQMFObject();    
}

void CPUWrapper::cleanupQMFObject(void)
{
    mgmt_object->resourceDestroy();
}

void CPUWrapper::syncQMFObject(void)
{
    mgmt_object->set_cpunum(cpunum);
    mgmt_object->set_corenum(corenum);
    mgmt_object->set_numcores(numcores);
    mgmt_object->set_model(model);
    mgmt_object->set_family(family);
    mgmt_object->set_cpuid_lvl(cpuid_lvl);
    mgmt_object->set_speed(speed);
    mgmt_object->set_cache(cache);
    mgmt_object->set_vendor(vendor);
    mgmt_object->set_flags(flags);
}

/**
 * void fillCPUInfo(vector <CPUWrapper*> &cpus, ManagementAgent *agent)
 * 
 * Takes in a vector of CPUWrapper object pointers and populates it with
 * CPUs found by querying /proc/cpuinfo. NOTE: This method is very sensitive
 * to the output format of /proc/cpuinfo.
 *
 * Throws a runtime error if file io is unsuccessful.
 */
void CPUWrapper::fillCPUInfo(vector<CPUWrapper*> &cpus, ManagementAgent *agent)
{
    string line;
    boost::smatch matches;
    boost::regex re("(.*\\S)\\s*:\\s*(\\S.*)");
    ifstream cpuinfo("/proc/cpuinfo", ios::in);
    if (!cpuinfo.is_open() || cpuinfo.fail())
        throw runtime_error("Unable to open /proc/cpuinfo");

    // Each line is a key:value pair. New processor
    // delimiter is the "processor" key.
    while (!cpuinfo.eof()) {
        getline(cpuinfo, line);
        if (boost::regex_match(line, matches, re)) {
            if (matches[1] == "processor") {
                // Start pulling data for a new processor
		        int cpunum;
		        int coreid;
		        int cpucores;
		        int model;
		        int family;
		        int cpuid_lvl;
		        double speed;
		        int cache;
		        string vendor;
		        string flags;
                // Get the cpu # from this line
                cpunum = boost::lexical_cast<int, string>(matches[2]);
                // And now grab the rest
                do {
                    getline(cpuinfo, line);
                    if (boost::regex_match(line, matches, re)) {

                        if (matches[1] == "core id") {
                            coreid = boost::lexical_cast<int>(matches[2]);
                        } else if (matches[1] == "cpu cores") {
                            cpucores = boost::lexical_cast<int>(matches[2]);
                        } else if (matches[1] == "model") {
                            model = boost::lexical_cast<int>(matches[2]);
                        } else if (matches[1] == "cpu family") {
                            family = boost::lexical_cast<int>(matches[2]);
                        } else if (matches[1] == "cpuid level") {
                            cpuid_lvl = boost::lexical_cast<int>(matches[2]);
                        } else if (matches[1] == "cpu MHz") {
                            speed = boost::lexical_cast<double>(matches[2]);
                        } else if (matches[1] == "cache size") {
                            string str = matches[2].str();
                            int space = str.find(' ');
                            cache = boost::lexical_cast<int>(str.substr(0, space));
                        } else if (matches[1] == "vendor_id") {
                            vendor = string(matches[2]);
                        } else if (matches[1] == "flags") {
                            flags = string(matches[2]);
                        }
        			}
    		    }
    		    while (line != "");

    		    // Got all the data. Add the CPU to our list
                CPUWrapper *cpu = new CPUWrapper(cpunum,
                                                coreid,
                                                cpucores,
                                                model,
                                                family,
                                                cpuid_lvl,
                                                speed,
                                                cache,
                                                vendor,
                                                flags);
    		    cpus.push_back(cpu);
    		}
	    }
    	else
            continue;
    }
	cpuinfo.close();
}
