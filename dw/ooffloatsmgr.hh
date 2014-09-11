#ifndef __DW_OOFFLOATSMGR_HH__
#define __DW_OOFFLOATSMGR_HH__

#include "outofflowmgr.hh"

namespace dw {

namespace oof {

class OOFFloatsMgr: public OutOfFlowMgr
{
   friend class WidgetInfo;

private:
   enum Side { LEFT, RIGHT };
   enum SFVType { GB, CB };

   Textblock *containingBlock;

   // These two values are redundant to TBInfo::wasAllocated and
   // TBInfo::allocation, for some special cases.
   bool containingBlockWasAllocated;
   core::Allocation containingBlockAllocation;

   class WidgetInfo: public lout::object::Object
   {
   private:
      bool wasAllocated;
      int xCB, yCB; // relative to the containing block
      int width, height;

      OOFFloatsMgr *oofm;
      core::Widget *widget;

   protected:
      OOFFloatsMgr *getOOFFloatsMgr () { return oofm; }

   public:
      WidgetInfo (OOFFloatsMgr *oofm, core::Widget *widget);

      inline bool wasThenAllocated () { return wasAllocated; }
      inline int getOldXCB () { return xCB; }
      inline int getOldYCB () { return yCB; }
      inline int getOldWidth () { return width; }
      inline int getOldHeight () { return height; }


      void update (bool wasAllocated, int xCB, int yCB, int width, int height);

      inline core::Widget *getWidget () { return widget; }
   };

   class Float: public WidgetInfo
   {
   public:
      class ComparePosition: public lout::object::Comparator
      {
      private:
         OOFFloatsMgr *oofm;
         Textblock *refTB;
         SFVType type; // actually only used for debugging

      public:
         ComparePosition (OOFFloatsMgr *oofm, Textblock *refTB, SFVType type)
         { this->oofm = oofm; this->refTB = refTB; this->type = type; }
         int compare(Object *o1, Object *o2);
      };

      class CompareSideSpanningIndex: public lout::object::Comparator
      {
      public:
         int compare(Object *o1, Object *o2);
      };

      class CompareGBAndExtIndex: public lout::object::Comparator
      {
      private:
         OOFFloatsMgr *oofm;
         SFVType type; // actually only used for debugging

      public:
         CompareGBAndExtIndex (OOFFloatsMgr *oofm, SFVType type)
         { this->oofm = oofm; this->type = type; }
         int compare(Object *o1, Object *o2);
      };

      Textblock *generatingBlock;
      int externalIndex;
      int yReq, yReal; // relative to generator, not container
      int indexGBList; /* Refers to TBInfo::leftFloatsGB or
                          TBInfo::rightFloatsGB, respectively. -1
                          initially. */
      int indexCBList; /* Refers to leftFloatsCB or rightFloatsCB,
                          respectively. -1 initially. */
      int sideSpanningIndex, mark;
      core::Requisition size;
      int cbLineBreakWidth; /* On which the calculation of relative sizes
                               is based. Height not yet used, and probably
                               not added before size redesign. */
      bool dirty, sizeChangedSinceLastAllocation;

      Float (OOFFloatsMgr *oofm, core::Widget *widget,
             Textblock *generatingBlock, int externalIndex);

      inline bool isNowAllocated () { return getWidget()->wasAllocated (); }
      inline int getNewXCB () { return getWidget()->getAllocation()->x -
            getOOFFloatsMgr()->containingBlockAllocation.x; }
      inline int getNewYCB () { return getWidget()->getAllocation()->y -
            getOOFFloatsMgr()->containingBlockAllocation.y; }
      inline int getNewWidth () { return getWidget()->getAllocation()->width; }
      inline int getNewHeight () { return getWidget()->getAllocation()->ascent +
            getWidget()->getAllocation()->descent; }
      void updateAllocation ();

      inline int *getIndexRef (SFVType type) {
         return type == GB ? &indexGBList : &indexCBList; }
      inline int getIndex (SFVType type) { return *(getIndexRef (type)); }
      inline void setIndex (SFVType type, int value) {
         *(getIndexRef (type)) = value; }

      void intoStringBuffer(lout::misc::StringBuffer *sb);

      bool covers (Textblock *textblock, int y, int h);
   };

   /**
    * This list is kept sorted.
    *
    * To prevent accessing methods of the base class in an
    * uncontrolled way, the inheritance is private, not public; this
    * means that all methods must be delegated (see iterator(), size()
    * etc. below.)
    *
    * TODO Update comment: still sorted, but ...
    *
    * More: add() and change() may check order again.
    */
   class SortedFloatsVector: private lout::container::typed::Vector<Float>
   {
   public:
      SFVType type;

   private:
      OOFFloatsMgr *oofm;
      Side side;

   public:
      inline SortedFloatsVector (OOFFloatsMgr *oofm, Side side, SFVType type) :
         lout::container::typed::Vector<Float> (1, false)
      { this->oofm = oofm; this->side = side; this->type = type; }

      int findFloatIndex (Textblock *lastGB, int lastExtIndex);
      int find (Textblock *textblock, int y, int start, int end);
      int findFirst (Textblock *textblock, int y, int h, Textblock *lastGB,
                     int lastExtIndex, int *lastReturn);
      int findLastBeforeSideSpanningIndex (int sideSpanningIndex);
      void put (Float *vloat);

      inline lout::container::typed::Iterator<Float> iterator()
      { return lout::container::typed::Vector<Float>::iterator (); }
      inline int size ()
      { return lout::container::typed::Vector<Float>::size (); }
      inline Float *get (int pos)
      { return lout::container::typed::Vector<Float>::get (pos); }
      inline void clear ()
      { lout::container::typed::Vector<Float>::clear (); }
   };

   class TBInfo: public WidgetInfo
   {
   public:
      int lineBreakWidth;
      int index; // position within "tbInfos"

      TBInfo *parent;
      int parentExtIndex;

      // These two values are set by sizeAllocateStart(), and they are
      // accessable also within sizeAllocateEnd() for the same
      // textblock, for which allocation and WAS_ALLOCATED is set
      // *after* sizeAllocateEnd(). See the two functions
      // wasAllocated(Widget*) and getAllocation(Widget*) (further
      // down) for usage.
      bool wasAllocated;
      core::Allocation allocation;

      // These two lists store all floats generated by this textblock,
      // as long as this textblock is not allocates.
      SortedFloatsVector *leftFloatsGB, *rightFloatsGB;

      TBInfo (OOFFloatsMgr *oofm, Textblock *textblock,
              TBInfo *parent, int parentExtIndex);
      ~TBInfo ();

      inline bool isNowAllocated () {
         return getOOFFloatsMgr()->wasAllocated (getTextblock ()); }
      inline int getNewXCB () {
         return getOOFFloatsMgr()->getAllocation (getTextblock ())->x -
            getOOFFloatsMgr()->containingBlockAllocation.x; }
      inline int getNewYCB () {
         return getOOFFloatsMgr()->getAllocation (getTextblock ())->y -
            getOOFFloatsMgr()->containingBlockAllocation.y; }
      inline int getNewWidth () {
         return getOOFFloatsMgr()->getAllocation (getTextblock ())->width; }
      inline int getNewHeight () {
         core::Allocation *allocation =
            getOOFFloatsMgr()->getAllocation (getTextblock ());
         return allocation->ascent + allocation->descent; }
      void updateAllocation ();

      inline Textblock *getTextblock () { return (Textblock*)getWidget (); }
   };

   // These two lists store all floats, in the order in which they are
   // defined. Only used for iterators.
   lout::container::typed::Vector<Float> *leftFloatsAll, *rightFloatsAll;

   // These two lists store all floats whose generators are already
   // allocated.
   SortedFloatsVector *leftFloatsCB, *rightFloatsCB;

   // These two attributes are used in the size allocation process;
   // see sizeAllocateStart and sizeAllocateEnd.
   int lastAllocatedLeftFloat, lastAllocatedRightFloat;

   lout::container::typed::HashTable<lout::object::TypedPointer
                                     <dw::core::Widget>, Float> *floatsByWidget;

   lout::container::typed::Vector<TBInfo> *tbInfos;
   lout::container::typed::HashTable<lout::object::TypedPointer <Textblock>,
                                     TBInfo> *tbInfosByTextblock;

   int lastLeftTBIndex, lastRightTBIndex, leftFloatsMark, rightFloatsMark;

   /**
    * Variant of Widget::wasAllocated(), which can also be used within
    * OOFM::sizeAllocateEnd().
    */
   inline bool wasAllocated (Textblock *textblock) {
      return getTextblock(textblock)->wasAllocated;
   }

   /**
    * Variant of Widget::getAllocation(), which can also be used
    * within OOFM::sizeAllocateEnd().
    */
   inline core::Allocation *getAllocation (Textblock *textblock) {
      return &(getTextblock(textblock)->allocation);
   }

   void moveExternalIndices (SortedFloatsVector *list, int oldStartIndex,
                             int diff);
   Float *findFloatByWidget (core::Widget *widget);

   void moveFromGBToCB (Side side);
   void sizeAllocateFloats (Side side, int newLastAllocatedFloat);
   int calcFloatX (Float *vloat, Side side, int gbX, int gbWidth,
                   int gbLineBreakWidth);

   bool hasRelationChanged (TBInfo *tbInfo,int *minFloatPos,
                            core::Widget **minFloat);
   bool hasRelationChanged (TBInfo *tbInfo, Side side, int *minFloatPos,
                            core::Widget **minFloat);
   bool hasRelationChanged (bool oldTBAlloc,
                            int oldTBx, int oldTBy, int oldTBw, int oldTBh,
                            int newTBx, int newTBy, int newTBw, int newTBh,
                            bool oldFlAlloc,
                            int oldFlx, int oldFly, int oldFlw, int oldFlh,
                            int newFlx, int newFly, int newFlw, int newFlh,
                            Side side, int *floatPos);

   void checkAllocatedFloatCollisions (Side side);

   bool doFloatsExceedCB (Side side);
   bool haveExtremesChanged (Side side);

   void drawFloats (SortedFloatsVector *list, core::View *view,
                    core::Rectangle *area);
   core::Widget *getFloatWidgetAtPoint (SortedFloatsVector *list, int x, int y,
                                        int level);

   bool collidesV (Float *vloat, Float *other, SFVType type, int *yReal);
   bool collidesH (Float *vloat, Float *other, SFVType type);

   void getFloatsListsAndSide (Float *vloat, SortedFloatsVector **listSame,
                               SortedFloatsVector **listOpp, Side *side);

   void getFloatsSize (core::Requisition *cbReq, Side side, int *width,
                       int *height);
   void getFloatsExtremes (core::Extremes *cbExtr, Side side, int *minWidth,
                           int *maxWidth);
   bool getFloatDiffToCB (Float *vloat, int *leftDiff, int *rightDiff);

   TBInfo *getTextblock (Textblock *textblock);
   int getBorder (Textblock *textblock, Side side, int y, int h,
                  Textblock *lastGB, int lastExtIndex);
   SortedFloatsVector *getFloatsListForTextblock (Textblock *textblock,
                                                  Side side);
   bool hasFloat (Textblock *textblock, Side side, int y, int h,
                  Textblock *lastGB, int lastExtIndex);

   int getFloatHeight (Textblock *textblock, Side side, int y, int h,
                       Textblock *lastGB, int lastExtIndex);

   int getClearPosition (Textblock *textblock, Side side);

   void ensureFloatSize (Float *vloat);

   inline static int createSubRefLeftFloat (int index) { return index << 1; }
   inline static int createSubRefRightFloat (int index)
   { return (index << 1) | 1; }

   inline static bool isSubRefLeftFloat (int ref)
   { return ref != -1 && (ref & 1) == 0; }
   inline static bool isSubRefRightFloat (int ref)
   { return ref != -1 && (ref & 1) == 1; }

   inline static int getFloatIndexFromSubRef (int ref)
   { return ref == -1 ? ref : (ref >> 1); }

public:
   OOFFloatsMgr (Textblock *containingBlock);
   ~OOFFloatsMgr ();

   void sizeAllocateStart (Textblock *caller, core::Allocation *allocation);
   void sizeAllocateEnd (Textblock *caller);
   void containerSizeChangedForChildren ();
   void draw (core::View *view, core::Rectangle *area);

   void markSizeChange (int ref);
   void markExtremesChange (int ref);
   core::Widget *getWidgetAtPoint (int x, int y, int level);

   static bool _isWidgetOutOfFlow (core::Widget *widget);
   static bool _isWidgetHandledByOOFM (core::Widget *widget);
   void addWidgetInFlow (Textblock *textblock, Textblock *parentBlock,
                         int externalIndex);
   int addWidgetOOF (core::Widget *widget, Textblock *generatingBlock,
                     int externalIndex);
   void moveExternalIndices (Textblock *generatingBlock, int oldStartIndex,
                             int diff);

   void tellPosition (core::Widget *widget, int yReq);

   void getSize (core::Requisition *cbReq, int *oofWidth, int *oofHeight);
   void getExtremes (core::Extremes *cbExtr,
                     int *oofMinWidth, int *oofMaxWidth);

   int getLeftBorder (Textblock *textblock, int y, int h, Textblock *lastGB,
                      int lastExtIndex);
   int getRightBorder (Textblock *textblock, int y, int h, Textblock *lastGB,
                       int lastExtIndex);

   bool hasFloatLeft (Textblock *textblock, int y, int h, Textblock *lastGB,
                      int lastExtIndex);
   bool hasFloatRight (Textblock *textblock, int y, int h, Textblock *lastGB,
                       int lastExtIndex);

   int getLeftFloatHeight (Textblock *textblock, int y, int h,
                           Textblock *lastGB, int lastExtIndex);
   int getRightFloatHeight (Textblock *textblock, int y, int h,
                            Textblock *lastGB, int lastExtIndex);

   bool affectsLeftBorder (core::Widget *widget);
   bool affectsRightBorder (core::Widget *widget);

   int getClearPosition (Textblock *textblock);

   bool dealingWithSizeOfChild (core::Widget *child);
   int getAvailWidthOfChild (core::Widget *child, bool forceValue);
   int getAvailHeightOfChild (core::Widget *child, bool forceValue);

   int getNumWidgets ();
   core::Widget *getWidget (int i);
};

} // namespace oof

} // namespace dw

#endif // __DW_OOFFLOATSMGR_HH__