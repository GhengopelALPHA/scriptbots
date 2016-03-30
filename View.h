#ifndef VIEW_H
#define VIEW_H

#include "Agent.h"
class View
{
public:
    virtual void drawAgent(const Agent &a, float x, float y, bool ghost= 0) = 0;
    virtual void drawCell(int x, int y, float quantity) = 0;
    virtual void drawData() = 0;
	virtual void drawStatic() = 0;
};

#endif // VIEW_H
