#include "mitkContour.h"

#include <mitkPointOperation.h>
#include <mitkOperationActor.h>

//#include <qapplication.h>//@todo because of quickimplementation UpdateAllWidgets
//#include <qwidgetlist.h>//@todo because of quickimplementation UpdateAllWidgets

#include <mitkInteractionConst.h>


//inline void UpdateAllWidgets()//@todo global quickimplementation. is to be changed into Modified Data...
//{
//    QWidgetList *list = QApplication::allWidgets();
//    QWidgetListIt it( *list );         // iterate over the widgets
//    QWidget * w;
//    while ( (w=it.current()) != 0 ) {  // for each widget...
//        ++it;
//        w->update();
//    }
//    delete list;                      // delete the list, not the widgets
//
//}

//##ModelId=3F0177E901BD
mitk::Contour::Contour() :
m_ContourPath (PathType::New()),
m_CurrentWindow ( NULL ),
m_BoundingBox (BoundingBoxType::New()),
m_Vertices ( BoundingBoxType::PointsContainer::New() )  
{
  m_Geometry3D->Initialize();
}

//##ModelId=3F0177E901BE
mitk::Contour::~Contour()
{}

void mitk::Contour::AddVertex(mitk::ITKPoint3D newPoint)
{
  BoundingBoxType::PointType p;
  p.CastFrom(newPoint);
  m_Vertices->InsertElement(m_Vertices->Size(), p);

  ContinuousIndexType idx;
  idx.CastFrom(newPoint);
  m_ContourPath->AddVertex(idx);
}

//##ModelId=3F0177E901BF
void mitk::Contour::ExecuteOperation(Operation* op)
{
  mitk::PointOperation * pointOp = dynamic_cast<mitk::PointOperation*>( op );
  if ( pointOp != NULL )
  {

    switch (op->GetOperationType())
    {
    case mitk::OpADD:
      {
        m_ContourPath = PathType::New();
        m_ContourPath->Initialize();
      }

    case mitk::OpMOVE:
      {
        //std::cout << "add vertex to contour" << std::endl;
        AddVertex(pointOp->GetPoint());
        UpdateOutputInformation();
        if (m_CurrentWindow != NULL) {
          m_CurrentWindow->Update();
        }
      }
    }
  }
}

//##ModelId=3F0177E901EE
void mitk::Contour::UpdateOutputInformation()
{
  float mitkBounds[6];
  if (m_Vertices->Size() == 0)  {
    mitkBounds[0] = 0.0;
    mitkBounds[1] = 0.0;
    mitkBounds[2] = 0.0;
    mitkBounds[3] = 0.0;
    mitkBounds[4] = 0.0;
    mitkBounds[5] = 0.0;
  }
  else
  {
    m_BoundingBox->ComputeBoundingBox();
    BoundingBoxType::BoundsArrayType tmp = m_BoundingBox->GetBounds();
    mitkBounds[0] = tmp[0];
    mitkBounds[1] = tmp[1];
    mitkBounds[2] = tmp[2];
    mitkBounds[3] = tmp[3];
    mitkBounds[4] = tmp[4];
    mitkBounds[5] = tmp[5];
  }
  m_Geometry3D->SetBoundingBox(mitkBounds);
}

//##ModelId=3F0177E901FB
void mitk::Contour::SetRequestedRegionToLargestPossibleRegion()
{
}
//##ModelId=3F0177E901FD
bool mitk::Contour::RequestedRegionIsOutsideOfTheBufferedRegion()
{
    return false;
}
//##ModelId=3F0177E901FF
bool mitk::Contour::VerifyRequestedRegion()
{
    return false;
}
//##ModelId=3F0177E9020B
void mitk::Contour::SetRequestedRegion(itk::DataObject*)
{
}

mitk::Contour::PathType::Pointer 
mitk::Contour::GetContourPath()
{
  return m_ContourPath;
}

void 
mitk::Contour::SetCurrentWindow(mitk::RenderWindow* rw)
{
  m_CurrentWindow = rw;
}
