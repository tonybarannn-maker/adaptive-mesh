#include "production_authority_types.hpp"
#include "production_transition_eligibility.hpp"

void consume(AdaptiveMesh::ProductionExecutionCapability);

void probe(
    const AdaptiveMesh::ProductionTransitionEligibilityDecision& decision)
{
    consume(decision);
}

int main() { return 0; }
