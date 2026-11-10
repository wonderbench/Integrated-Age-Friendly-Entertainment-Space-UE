#include "ElderSplinePath.h"
#include "Components/SplineComponent.h"

AElderSplinePath::AElderSplinePath()
{
	PrimaryActorTick.bCanEverTick=false;
	Spline=CreateDefaultSubobject<USplineComponent>(TEXT("RouteSpline"));
	SetRootComponent(Spline);
	Spline->bEditableWhenInherited=true;
	Spline->bInputSplinePointsToConstructionScript=true;
	// Constructor defaults only: never rebuild points in OnConstruction, which would erase edits.
	Spline->ClearSplinePoints(false);
	Spline->AddSplinePoint(FVector::ZeroVector,ESplineCoordinateSpace::Local,false);
	Spline->AddSplinePoint(FVector(400,0,0),ESplineCoordinateSpace::Local,false);
	Spline->AddSplinePoint(FVector(800,200,0),ESplineCoordinateSpace::Local,true);
}
