
#include <sst/core/sst_config.h>

#include "junorandacc.h"
#include "junogenrandresp.h"

using namespace SST::Juno;

JunoRandAccelerator::JunoRandAccelerator(SST::ComponentId_t id, SST::Params& params) :
	Component(id) {

	const int verbosity = params.find<int>("verbose", 0);

	output = new SST::Output("JunoRandAccel: ", verbosity, 0, Output::STDOUT);

	const uint64_t rngSeed = params.find<uint64_t>("seed", static_cast<uint64_t>(101010101));
	output->verbose(CALL_INFO, 1, 0, "Creating RNG with seed %" PRIu64 "...", rngSeed);
	rng = new MersenneRNG( rngSeed );

	output->verbose(CALL_INFO, 1, 0, "Creating link to CPU...\n");
	cpuLink = configureLink("cpulink", new Event::Handler<JunoRandAccelerator>(
			this, &JunoRandAccelerator::handleGenerateReq));

	if( NULL == cpuLink ) {
		output->fatal(CALL_INFO, -1, "Error: unable to configure link for CPU.\n");
	}

	std::string genLinkTime = params.find<std::string>("gentime", "10ns");

	output->verbose(CALL_INFO, 1, 0, "Creating an internal link for timing random generation (timing=%s)...\n", genLinkTime.c_str());
	selfGenerateLink = configureSelfLink("genlink", genLinkTime,
		new Event::Handler<JunoRandAccelerator>(this, &JunoRandAccelerator::handleRNGenerated));

	if( NULL == selfGenerateLink ) {
		output->fatal(CALL_INFO, -1, "Error: unable to create self link for timing.\n");
	}

	output->verbose(CALL_INFO, 1, 0, "Ready to use.\n");
}

JunoRandAccelerator::~JunoRandAccelerator() {
	delete rng;
	delete output;
}

void JunoRandAccelerator::handleGenerateReq(SST::Event* ev) {
	output->verbose(CALL_INFO, 4, 0, "Received request to generate a random number.\n");

	JunoGenerateRandEvent* genEv = dynamic_cast<JunoGenerateRandEvent*>(ev);

	if( NULL == genEv ) {
		output->fatal(CALL_INFO, -1, "Error: unable to convert event from CPU\n");
	}

	output->verbose(CALL_INFO, 4, 0, "Request ID=%d\n", genEv->getID());

	// Pass the event on in the self timing link, this will return when
	// the event gets generated
	selfGenerateLink->send( genEv );
}

void JunoRandAccelerator::handleRNGenerated(SST::Event* ev) {
	output->verbose(CALL_INFO, 4, 0, "Generated a random number, ready to return response.\n");

	JunoGenerateRandEvent* genEv = dynamic_cast<JunoGenerateRandEvent*>(ev);

	if( NULL == genEv ) {
		output->fatal(CALL_INFO, -1, "Error: unable to convert event from CPU\n");
	}

	output->verbose(CALL_INFO, 4, 0, "Request ID=%d\n", genEv->getID());

	JunoGenerateRandRespEvent* resp = new JunoGenerateRandRespEvent(genEv->getID(),
		rng->generateNextInt64());
	cpuLink->send(resp);

	if( NULL != ev) {
		delete ev;
	}
}
