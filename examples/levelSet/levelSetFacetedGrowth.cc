/*
 * Advanced Simulation Library <http://asl.org.il>
 * 
 * Copyright 2015 Avtech Scientific <http://avtechscientific.com>
 *
 *
 * This file is part of Advanced Simulation Library (ASL).
 *
 * ASL is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, version 3 of the License.
 *
 * ASL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with ASL. If not, see <http://www.gnu.org/licenses/>.
 *
 */


/**
	\example levelSetFacetedGrowth.cc
	Required input file: default parameters - levelSetFacetedGrowth.ini
*/

#include <aslDataInc.h>
#include <writers/aslVTKFormatWriters.h>
#include <num/aslLSFacetedGrowth.h>
#include <utilities/aslTimer.h>
#include <utilities/aslParametersManager.h>
#include <math/aslTemplates.h>
#include <acl/aclMath/aclVectorOfElements.h>
#include <acl/aclUtilities.h>
#include <aslGeomInc.h>

typedef float FlT;
//typedef double FlT;
typedef asl::UValue<FlT> Param;
acl::TypeID type(acl::typeToTypeID<FlT>()); 


int main(int argc, char* argv[])
{
	asl::ApplicationParametersManager appParamsManager("levelSetFacetedGrowth",
	                                                   "1.0");

	asl::Parameter<asl::AVec<int>> size(asl::makeAVec<int>(100, 100, 100), "size", "size");
	asl::Parameter<FlT> dx(1.0, "dx", "dx");
	asl::Parameter<FlT> dt(1.0, "dt", "dt");
	asl::Parameter<FlT> superS(0.2, "superS", "Super saturation");
	asl::Parameter<FlT> radius(10.5, "radius", "Initial radius");
	asl::Parameter<FlT> betaSt(0.5, "beta_step", "Kinetic coefficient for step");
	asl::Parameter<FlT> betaDisl(0.0, "beta_dislocation", "Kinetic coefficient for dislocation");
	asl::Parameter<FlT> betaRough(0.1, "beta_rough", "Kinetic coefficient for rough region");

	asl::Parameter<map<string, asl::AVec<FlT>>> cr_directions_p("cr_direction_*",
	                                                            "Crystallographic directions");
	
	asl::Parameter<cl_uint> nIterations(100, "nIterations", "Number of iterations");
	asl::Parameter<cl_uint> nItOut(10, "nItOut", "Number of iterations for output");

	appParamsManager.load(argc, argv);

	std::cout << "Data initialization... ";

	asl::Block block(size.v(), dx.v());
	auto levelSet(asl::generateDataContainerACL_SP<FlT>(block, 1, 1u));

	asl::AVec<> center(asl::AVec<FlT>(size.v())*FlT(.5));
		
	auto sphere1(generateDFSphere(radius.v(), center*.8));
	auto sphere2(generateDFSphere(radius.v(), center*1.2));
	asl::initData(levelSet, normalize(sphere1 | sphere2, dx.v()));
	
	auto superSaturation(asl::generateDataContainerConst_SP(block, superS.v(), 1u));

	
	asl::WriterVTKXML writer(appParamsManager.getDir() + "levelSetFacetedGrowth");
	writer.addScalars("levelSet", *levelSet);
	
	std::cout << "Finished" << endl;
	
	std::cout << "Numerics initialization... " << flush;

	auto lsNum(std::make_shared<asl::LSFacetedGrowth>(levelSet, superSaturation));

	lsNum->crystallography.betaRough = betaRough.v();
	for (auto it(cr_directions_p.v().begin()); it != cr_directions_p.v().end(); ++it)
		lsNum->crystallography.addFacet(asl::AVec<double>(it->second), betaSt.v(), betaDisl.v());
	
	lsNum->init();

	std::cout << "Finished" << endl;
	std::cout << "Computing...";
	asl::Timer timer;

	writer.write();
	
	timer.start();
	for (unsigned int i(0); i < nIterations.v(); ++i)
	{
		lsNum->execute();
		if (!(i % nItOut.v()))
			writer.write();		
	}
	timer.stop();
	
	cout << "Finished" << endl;	

	cout << "Computation statistic:" << endl;
	cout << "Real Time = " << timer.realTime() << "; Processor Time = "
		 << timer.processorTime() << "; Processor Load = "
		 << timer.processorLoad() * 100 << "%" << endl;

	return 0;
}
