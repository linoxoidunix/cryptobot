#include "aot/bus/bus.h"


void sp::ImplementationEventA::Accept(sp::bus::Component* comp){
    comp->HandleEvent(this);
}

void sp::ImplementationEventB::Accept(sp::bus::Component* comp){
    comp->HandleEvent(this);
}

void sp::ImplementationEventC::Accept(sp::bus::Component* comp){
    comp->HandleEvent(this);
}


