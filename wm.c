#include "wm.h"
#include "debug.h"
#include "int.h"
#include "gui.h"
#include "str.h"

void WMAddWindow(WM *WinM, Window *Win){
    if (!WinM || !Win) return;
    
    Win->Next = NULL;
    
    if (WinM->Tail == NULL) {
        WinM->Head = Win;
        WinM->Tail = Win;
        Win->Pre = NULL;
    } else {
        WinM->Tail->Next = Win;
        Win->Pre = WinM->Tail;
        WinM->Tail = Win;
    }
}

void WMDelWindow(WM *WinM, const char *Title, u64 SomeID){
    if (!WinM) return;
    if (!WinM->Head) {
        DebugStr("Window Not Found.\n");
        return;
    }
    
    int UseTitle = (Title != NULL);
    Window* Node = WinM->Head;
    
    while (Node != NULL) {
        int match = UseTitle ? StrIs(Title, Node->Title) 
                             : (SomeID == Node->SomeID);
        if (match) break;
        Node = Node->Next;
    }
    
    if (!Node) {
        DebugStr("Window Not Found.\n");
        return;
    }
    
    if (Node == WinM->Head) {
        WinM->Head = Node->Next;
        if (WinM->Head) WinM->Head->Pre = NULL;
    } else {
        Node->Pre->Next = Node->Next;
    }
    
    if (Node == WinM->Tail) {
        WinM->Tail = Node->Pre;
        if (WinM->Tail) WinM->Tail->Next = NULL;
    } else {
        Node->Next->Pre = Node->Pre;
    }
    
    DebugStr("Del Window.\n");
}

void WMUpWindow(WM* WinM, const char* Title, u64 SomeID) {
    if (!WinM) return;
    if (!WinM->Head) return;
    
    int UseTitle = (Title != NULL);
    Window* Target = NULL;
    
    Window* Node = WinM->Head;
    while (Node != NULL) {
        if (UseTitle) {
            if (StrIs(Title, Node->Title)) { Target = Node; break; }
        } else {
            if (SomeID == Node->SomeID) { Target = Node; break; }
        }
        Node = Node->Next;
    }
    
    if (!Target) return;
    if (Target == WinM->Tail) return;

    if (Target == WinM->Head) {
        WinM->Head = Target->Next;
        if (WinM->Head) WinM->Head->Pre = NULL;
    } else {
        Target->Pre->Next = Target->Next;
    }
    
    if (Target == WinM->Tail) {
        WinM->Tail = Target->Pre;
    } else {
        Target->Next->Pre = Target->Pre;
    }
    Target->Pre = WinM->Tail;
    Target->Next = NULL;
    WinM->Tail->Next = Target;
    WinM->Tail = Target;
    
    DebugStr("Window Moved to Top: Title: ");
    DebugStr(WinM->Tail->Title);
    DebugStr(" SomeID: 0x");
    DebugU64(WinM->Tail->SomeID);
    DebugChar('\n');
}

Window* WMWindowsTitleOverlapping(WM* WinM, Window* WinSelf, u64 TitleHight, u64 PointX, u64 PointY){
    if (WinM == NULL || WinSelf == NULL) return NULL;

    if (PointX >= WinSelf->Base.w || PointY >= TitleHight) {
        return NULL;
    }

    u64 AbsX = WinSelf->Base.x + PointX;
    u64 AbsY = WinSelf->Base.y + PointY;

    Window* Hit = NULL;
    Window* Node = WinM->Tail;
    while (Node != NULL) {
        if (Node != WinSelf && Node->Base.Visible && !Node->Hide && Node!=WinSelf) {
            u64 NodeL = Node->Base.x;
            u64 NodeT = Node->Base.y;
            u64 NodeR = Node->Base.x + Node->Base.w;
            u64 NodeB = Node->Base.y + TitleHight;

            if (AbsX >= NodeL && AbsX < NodeR &&
                AbsY >= NodeT && AbsY < NodeB) {
                Hit = Node;
            }
        }
        Node = Node->Pre;
    }

    return Hit;
}

void WMPollDraw(WM* WinM){
    if (!WinM) return;
    Window* Node = WinM->Head;
    while (Node != NULL) {
        Window* Next = Node->Next;
        if (Node->Base.Draw) {
            Node->Base.Draw(Node);
        }
        Node = Next;
    }
}

void WMPollPoll(WM* WinM){
    if (!WinM) return;

    Window* CheckNode = WinM->Tail;
    while (CheckNode != NULL) {
        if (CheckNode->IsDragging) {
            if (CheckNode != WinM->Tail) {
                WMUpWindow(WinM, NULL, CheckNode->SomeID);
            }
            break;
        }
        CheckNode = CheckNode->Pre;
    }

    Window* DraggingWindow = NULL;
    Window* Temp = WinM->Tail;
    while (Temp != NULL) {
        if (Temp->IsDragging) {
            DraggingWindow = Temp;
            break;
        }
        Temp = Temp->Pre;
    }

    if (DraggingWindow != NULL) {
        if (DraggingWindow->Base.Poll) {
            DraggingWindow->Base.Poll(DraggingWindow);
        }
        return;
    }

    Window* Node = WinM->Tail;
    while (Node != NULL) {
        Window* Next = Node->Pre;
        if (Node->Base.Poll) {
            Node->Base.Poll(Node);
        }
        Node = Next;
    }
}
