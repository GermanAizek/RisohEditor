// RubberBand.hpp -- Rubber Band for Win32
//////////////////////////////////////////////////////////////////////////////

#ifndef RUBBER_BAND_HPP_
#define RUBBER_BAND_HPP_    2   /* Version 2 */

#include "MWindowBase.hpp"

struct RubberBandTarget;
struct RubberBand;

//////////////////////////////////////////////////////////////////////////////

struct RubberBand : MWindowBase
{
    HRGN m_hRgn;
    MWindowBase  m_target;
    enum { m_nGripSize = 3 };

    RubberBand() : m_hRgn(NULL)
    {
    }

    BOOL CreateDx(HWND hwndParent, BOOL bVisible = FALSE,
                  INT x = 0, INT y = 0, INT cx = 0, INT cy = 0)
    {
        return CreateWindowDx(hwndParent, NULL,
            (bVisible ? WS_VISIBLE : 0) | WS_POPUP | WS_THICKFRAME,
            WS_EX_TOOLWINDOW | WS_EX_TOPMOST, x, y, cx, cy);
    }

    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
            HANDLE_MSG(hwnd, WM_MOVE, OnMove);
            HANDLE_MSG(hwnd, WM_SIZE, OnSize);
            HANDLE_MSG(hwnd, WM_NCCALCSIZE, OnNCCalcSize);
            HANDLE_MSG(hwnd, WM_PAINT, OnPaint);
            HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
            HANDLE_MSG(hwnd, WM_NCHITTEST, OnNCHitTest);
            HANDLE_MSG(hwnd, WM_SETCURSOR, OnSetCursor);
            HANDLE_MSG(hwnd, WM_TIMER, OnTimer);
            HANDLE_MSG(hwnd, WM_CLOSE, OnClose);
            HANDLE_MSG(hwnd, WM_NCRBUTTONDOWN, OnNCRButtonDown);
            HANDLE_MSG(hwnd, WM_KEYDOWN, OnKey);
            HANDLE_MSG(hwnd, WM_ACTIVATE, OnActivate);
        default:
            return DefaultProcDx(hwnd, uMsg, wParam, lParam);
        }
        return 0;
    }

    void OnActivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized)
    {
        if (state == WA_ACTIVE || state == WA_CLICKACTIVE)
        {
            if (m_target)
            {
                ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
                FitToTarget();
            }
        }
        else
        {
            if (GetAncestorDx(hwndActDeact) != GetAncestorDx(m_hwnd))
            {
                ShowWindow(m_hwnd, SW_HIDE);
            }
        }
    }

    void OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
    {
        FORWARD_WM_KEYDOWN(GetParent(hwnd), vk, cRepeat, flags, SendMessage);
    }

    void OnNCRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest)
    {
        if (m_target)
        {
            FORWARD_WM_CONTEXTMENU(GetParent(m_target), m_target, x, y, PostMessage);
        }
    }

    void OnTimer(HWND hwnd, UINT id)
    {
        if (id == 999)
        {
            KillTimer(hwnd, 999);
            HRGN hRgn = GetRgnOrDrawOrHitTest(hwnd);
            SetRgn(hRgn);
        }
    }

    void OnClose(HWND hwnd)
    {
        PostMessage(GetWindow(hwnd, GW_OWNER), WM_CLOSE, 0, 0);
    }

    BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
    {
        assert(!m_target);
        m_target.m_hwnd = NULL;

        HRGN hRgn = GetRgnOrDrawOrHitTest(hwnd);
        SetRgn(hRgn);
        return TRUE;
    }

    void GetIdealClientRect(LPRECT prc)
    {
        GetClientRect(m_hwnd, prc);
        InflateRect(prc, -2 * m_nGripSize, -2 * m_nGripSize);
    }

    void FitToBand()
    {
        if (!m_target)
        {
            BringWindowToTop(m_hwnd);
            return;
        }

        RECT rc;
        GetIdealClientRect(&rc);
        MapWindowRect(m_hwnd, GetParent(m_target), &rc);

        MoveWindow(m_target, rc.left, rc.top,
            rc.right - rc.left, rc.bottom - rc.top, TRUE);
        BringWindowToTop(m_hwnd);
        InvalidateRect(m_hwnd, NULL, TRUE);
    }

    void FitToTarget()
    {
        if (!m_target)
        {
            BringWindowToTop(m_hwnd);
            return;
        }

        RECT rc;
        GetWindowRect(m_target, &rc);
        if ((GetWindowStyle(m_hwnd) & WS_CHILD) && GetParent(m_hwnd))
        {
            MapWindowRect(NULL, GetParent(m_hwnd), &rc);
        }

        InflateRect(&rc, 2 * m_nGripSize, 2 * m_nGripSize);

        MoveWindow(m_hwnd, rc.left, rc.top,
            rc.right - rc.left, rc.bottom - rc.top, TRUE);
        BringWindowToTop(m_hwnd);
        InvalidateRect(m_hwnd, NULL, TRUE);
    }

    void OnDestroy(HWND hwnd)
    {
        DeleteObject(m_hRgn);
        m_hRgn = NULL;
        m_target.m_hwnd = NULL;
    }

    void GetRect(HWND hwnd, LPRECT prc)
    {
        GetWindowRect(hwnd, prc);
        OffsetRect(prc, -prc->left, -prc->top);
    }

    void SetRgn(HRGN hRgn, BOOL bUpdating = FALSE)
    {
        if (!bUpdating)
        {
            RECT rc;
            GetIdealClientRect(&rc);
            HRGN hClientRgn = CreateRectRgnIndirect(&rc);
            UnionRgn(hRgn, hRgn, hClientRgn);
            DeleteObject(hClientRgn);
        }

        HRGN hRgnOld = m_hRgn;
        SetWindowRgn(m_hwnd, hRgn, FALSE);
        m_hRgn = hRgn;

        DeleteObject(hRgnOld);
    }

    void OnPaint(HWND hwnd)
    {
        PAINTSTRUCT ps;
        HDC hDC = BeginPaint(hwnd, &ps);
        if (hDC)
        {
            HPEN hPenOld = SelectPen(hDC, GetStockPen(BLACK_PEN));
            HBRUSH hbrOld = SelectBrush(hDC, GetStockBrush(WHITE_BRUSH));
            {
                GetRgnOrDrawOrHitTest(hwnd, hDC, NULL);
            }
            SelectBrush(hDC, hbrOld);
            SelectPen(hDC, hPenOld);

            if (m_target)
            {
                RECT rc;
                GetIdealClientRect(&rc);

                RECT rcParent;
                HWND hParent = GetParent(m_target);
                GetWindowRect(hParent, &rcParent);
                MapWindowRect(NULL, m_hwnd, &rcParent);
                IntersectClipRect(hDC, rcParent.left, rcParent.top,
                                       rcParent.right, rcParent.bottom);

                SetWindowOrgEx(hDC, -rc.left, -rc.top, NULL);
                SendMessage(m_target, WM_PRINT, (WPARAM)hDC,
                            PRF_CHILDREN | PRF_CLIENT | PRF_NONCLIENT);
            }

            EndPaint(hwnd, &ps);
        }
    }

    UINT OnNCHitTest(HWND hwnd, int x, int y)
    {
        RECT rc;
        GetWindowRect(hwnd, &rc);
        x -= rc.left;
        y -= rc.top;

        POINT pt = { x, y };
        HRGN hRgn = GetRgnOrDrawOrHitTest(hwnd, NULL, &pt);
        return (UINT)(UINT_PTR)hRgn;
    }

    BOOL OnSetCursor(HWND hwnd, HWND hwndCursor, UINT codeHitTest, UINT msg)
    {
        switch (codeHitTest)
        {
        case HTTOPLEFT:         SetCursor(LoadCursor(NULL, IDC_SIZENWSE)); break;
        case HTLEFT:            SetCursor(LoadCursor(NULL, IDC_SIZEWE)); break;
        case HTBOTTOMLEFT:      SetCursor(LoadCursor(NULL, IDC_SIZENESW)); break;
        case HTTOP:             SetCursor(LoadCursor(NULL, IDC_SIZENS)); break;
        case HTBOTTOM:          SetCursor(LoadCursor(NULL, IDC_SIZENS)); break;
        case HTTOPRIGHT:        SetCursor(LoadCursor(NULL, IDC_SIZENESW)); break;
        case HTRIGHT:           SetCursor(LoadCursor(NULL, IDC_SIZEWE)); break;
        case HTBOTTOMRIGHT:     SetCursor(LoadCursor(NULL, IDC_SIZENWSE)); break;
        default:                SetCursor(LoadCursor(NULL, IDC_ARROW)); break;
        }
        return TRUE;
    }

    void InvalidateClient()
    {
        HRGN hRgn = GetRgnOrDrawOrHitTest(m_hwnd);
        SetRgn(hRgn, TRUE);

        RECT rc;
        GetIdealClientRect(&rc);

        HWND hTargetParent = GetParent(m_target);
        MapWindowRect(m_hwnd, hTargetParent, &rc);
        InvalidateRect(hTargetParent, &rc, TRUE);

        GetIdealClientRect(&rc);
        MapWindowRect(m_hwnd, m_target, &rc);
        InvalidateRect(m_target, &rc, TRUE);

        BringWindowToTop(m_hwnd);
        SetTimer(m_hwnd, 999, 100, NULL);
    }

    void OnMove(HWND hwnd, int x, int y)
    {
        if (m_hwnd)
        {
            FitToBand();
        }

        InvalidateClient();
    }

    void OnSize(HWND hwnd, UINT state, int cx, int cy)
    {
        if (m_hwnd)
        {
            FitToBand();
        }

        InvalidateClient();
    }

    HRGN GetRgnOrDrawOrHitTest(HWND hwnd, HDC hDC = NULL, LPPOINT ppt = NULL)
    {
        RECT rc;
        GetRect(hwnd, &rc);
        INT cx = rc.right - rc.left;
        INT cy = rc.bottom - rc.top;

        INT ax[] = { m_nGripSize, cx / 2, cx - m_nGripSize };
        INT ay[] = { m_nGripSize, cy / 2, cy - m_nGripSize };
        INT ahits[] =
        {
            HTTOPLEFT,    HTTOP,    HTTOPRIGHT,
            HTLEFT,                 HTRIGHT,
            HTBOTTOMLEFT, HTBOTTOM, HTBOTTOMRIGHT
        };

        HRGN hRgn = NULL;
        if (hDC == NULL && ppt == NULL)
            hRgn = CreateRectRgn(0, 0, 0, 0);

        for (INT k = 0, n = 0; k < 3; ++k)
        {
            for (INT i = 0; i < 3; ++i)
            {
                if (i == 1 && k == 1)
                    continue;

                if (hDC)
                {
                    ::Rectangle(hDC,
                        ax[i] - m_nGripSize, ay[k] - m_nGripSize,
                        ax[i] + m_nGripSize, ay[k] + m_nGripSize);
                }
                else if (ppt)
                {
                    RECT rect;
                    SetRect(&rect,
                        ax[i] - m_nGripSize, ay[k] - m_nGripSize,
                        ax[i] + m_nGripSize, ay[k] + m_nGripSize);
                    if (PtInRect(&rect, *ppt))
                    {
                        return (HRGN)(INT_PTR)ahits[n];
                    }
                }
                else
                {
                    HRGN hRgn2 = CreateRectRgn(
                        ax[i] - m_nGripSize, ay[k] - m_nGripSize,
                        ax[i] + m_nGripSize, ay[k] + m_nGripSize);
                    UnionRgn(hRgn, hRgn, hRgn2);
                    DeleteObject(hRgn2);
                }
                ++n;
            }
        }

        if (ppt)
        {
            hRgn = (HRGN)(INT_PTR)HTCAPTION;
        }

        return hRgn;
    }

    virtual LPCTSTR GetWndClassNameDx() const
    {
        return TEXT("katahiromz's Rubber Band Class");
    }

    virtual void ModifyWndClassDx(WNDCLASSEX& wcx)
    {
        MWindowBase::ModifyWndClassDx(wcx);
        wcx.hIcon = NULL;
        wcx.hbrBackground = GetStockBrush(NULL_BRUSH);
        wcx.hIconSm = NULL;
    }

    UINT OnNCCalcSize(HWND hwnd, BOOL fCalcValidRects, NCCALCSIZE_PARAMS *lpcsp)
    {
        return WVR_REDRAW;
    }

    void SetTarget(HWND hwndTarget)
    {
        m_target.m_hwnd = hwndTarget;
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef RUBBER_BAND_HPP_

//////////////////////////////////////////////////////////////////////////////
