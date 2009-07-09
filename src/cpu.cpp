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
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>

#include <pcre.h>

#include "cpu.h"
#include "qmf/com/redhat/matahari/CPU.h"

using namespace std;
namespace _qmf = qmf::com::redhat::matahari;

template<typename targetType> targetType convert(const std::string& str)
{
    istringstream i(str);
    targetType t;
    char c;
    if (!(i >> t))
        throw invalid_argument("Conversion failure for " + str);
    return t;
} 

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
    string regexstr = "(.*\\S)\\s*:\\s*(\\S.*)";
    int desiredmatches = 3; // Match string and two captured substrings
    int matchArraySize = desiredmatches * 3;
    int results[matchArraySize]; // pcre requires this much
    const char *pcre_err;
    int pcre_err_offset;
    int matched;
    pcre *regex;

    regex = pcre_compile(regexstr.c_str(), // input
                         0,                // no options
                         &pcre_err,        // where to place static error str
                         &pcre_err_offset, // index in regex string of error
                         NULL              // use the default charset
                        );
    if (!regex) {
        ostringstream err;
        err << "Error: Bad regex: " << regexstr << endl;
        err << "Error was: " << pcre_err << " at " << pcre_err_offset << endl;
        throw runtime_error(err.str());
    }

    ifstream cpuinfo("/proc/cpuinfo", ios::in);
    if (!cpuinfo.is_open() || cpuinfo.fail())
        throw runtime_error("Unable to open /proc/cpuinfo");

    // Each line is a key:value pair. New processor
    // delimiter is the "processor" key.
    while (!cpuinfo.eof()) {
        getline(cpuinfo, line);
        int match = pcre_exec(regex,         // Regex
                              NULL,          // No extra optimizations
                              line.c_str(),  // Input
                              line.length(), // Input length
                              0,             // Start offset
                              PCRE_NOTEMPTY, // options bitvector
                              results,       // Results vector
                              matchArraySize // Vector size
                              );

        if (match == desiredmatches) {
            if (line.substr(results[2], results[3] - results[2]) == "processor") {
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
                cpunum = convert<int>(line.substr(results[4], 
                                       results[5] - results[4]));
                // And now grab the rest
                do {
                    getline(cpuinfo, line);
                    match = pcre_exec(regex,         // Regex
                                      NULL,          // No extra optimizations
                                      line.c_str(),  // Input
                                      line.length(), // Input length
                                      0,             // Start offset
                                      PCRE_NOTEMPTY, // options bitvector
                                      results,       // Results vector
                                      matchArraySize // Vector size
				                      );

                    if (match == desiredmatches) {
                        string key = line.substr(results[2], 
                                                 results[3] - results[2]);

                        string value = line.substr(results[4], 
                                                    results[5] - results[4]);

                        if (key == "core id") {
                            coreid = convert<int>(value);
                        } else if (key == "cpu cores") {
                            cpucores = convert<int>(value);
                        } else if (key == "model") {
                            model = convert<int>(value);
                        } else if (key == "cpu family") {
                            family = convert<int>(value);
                        } else if (key == "cpuid level") {
                            cpuid_lvl = convert<int>(value);
                        } else if (key == "cpu MHz") {
                            speed = convert<int>(value);
                        } else if (key == "cache size") {
                            int space = value.find(' ');
                            cache = convert<int>(value.substr(0, space));
                        } else if (key == "vendor_id") {
                            vendor = value;
                        } else if (key == "flags") {
                            flags = value;
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

