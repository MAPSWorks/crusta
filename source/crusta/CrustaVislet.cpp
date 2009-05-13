#include <crusta/CrustaVislet.h>

#include <Vrui/VisletManager.h>

#include <crusta/Crusta.h>

BEGIN_CRUSTA

//- CrustaVisletFactory --------------------------------------------------------

CrustaVisletFactory::
CrustaVisletFactory(Vrui::VisletManager& visletManager) :
    VisletFactory("CrustaVislet", visletManager)
{
    CrustaVislet::factory = this;
}

CrustaVisletFactory::
~CrustaVisletFactory()
{
    CrustaVislet::factory = NULL;
}


Vrui::Vislet* CrustaVisletFactory::
createVislet(int numVisletArguments, const char* const visletArguments[]) const
{
    return new CrustaVislet;
}

void CrustaVisletFactory::
destroyVislet(Vrui::Vislet* vislet) const
{
    delete vislet;
}

extern "C" void
resolveCrustaVisletDependencies(
    Plugins::FactoryManager<Vrui::VisletFactory>& manager)
{
}
extern "C" Vrui::VisletFactory*
createCrustaVisletFactory(
    Plugins::FactoryManager<Vrui::VisletFactory>& manager)
{
    //get a handle to the vislet manager
    Vrui::VisletManager* visletManager =
        static_cast<Vrui::VisletManager*>(&manager);
    //create factory object and insert it into class hierarchy
    CrustaVisletFactory* crustaFactory =new CrustaVisletFactory(*visletManager);
    //return factory object
    return crustaFactory;
}
extern "C" void
destroyCrustaVisletFactory(Vrui::VisletFactory* factory)
{
    delete factory;
}

//- CrustaVislet ---------------------------------------------------------------

CrustaVisletFactory* CrustaVislet::factory = NULL;

CrustaVislet::
CrustaVislet() :
    crusta(new Crusta)
{}
CrustaVislet::
~CrustaVislet()
{
    delete crusta;
}

Vrui::VisletFactory* CrustaVislet::
getFactory() const
{
    return factory;
}

void CrustaVislet::
frame()
{
    crusta->frame();
}

void CrustaVislet::
display(GLContextData& contextData) const
{
    crusta->display(contextData);
}


END_CRUSTA