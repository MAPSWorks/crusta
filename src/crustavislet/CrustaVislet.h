#ifndef _CrustaVislet_H_
#define _CrustaVislet_H_

#include <Vrui/Vislet.h>

#include <crustacore/basics.h>

namespace Vrui {
    class VisletManager;
}


namespace crusta {


class Crusta;

///\todo comment properly
class CrustaVisletFactory : public Vrui::VisletFactory
{
    friend class CrustaVislet;

public:
    CrustaVisletFactory(Vrui::VisletManager& visletManager);
    virtual ~CrustaVisletFactory();

//- inherited from VisletFactory
public:
    virtual Vrui::Vislet* createVislet(
        int numVisletArguments, const char* const visletArguments[]) const;
    virtual void destroyVislet(Vrui::Vislet* vislet) const;
};

/**
    The main controlling component of the 'Crusta' vislet

    ///\todo what does it do, etc.
*/
class CrustaVislet : public Vrui::Vislet
{
    friend class CrustaVisletFactory;

public:
    CrustaVislet(int numArguments, const char* const arguments[]);
    ~CrustaVislet();

private:
    /** handle to the factory object for this class */
    static CrustaVisletFactory* factory;

    /** the crusta instance */
    Crusta* crusta;
    
//- inherited from Vislet
public:
    virtual Vrui::VisletFactory* getFactory() const;

    virtual void frame();
    virtual void display(GLContextData& contextData) const;
};


} //namespace crusta


#endif //_CrustaVislet_H_
