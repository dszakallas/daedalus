#pragma once

#include <QuartzCore/QuartzCore.h>

struct Simulator {
    Simulator();
private:
    CFTimeInterval startTime;
};

Simulator::Simulator() {
    startTime = CACurrentMediaTime();
}




