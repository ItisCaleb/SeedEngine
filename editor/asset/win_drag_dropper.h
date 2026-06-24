#ifndef _SEED_WIN_DRAG_DROPPER_H_
#define _SEED_WIN_DRAG_DROPPER_H_

#include <oleidl.h>
#include <windows.h>
#include <ole2.h>
#include <shlobj.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <string>
#include <vector>
#include "core/types.h"

namespace Seed {
class WindowDropTarget : public ::IDropTarget {
        ULONG ref_count = 1;
        bool dragging = false;

    public:
        ULONG STDMETHODCALLTYPE AddRef() override { return ++ref_count; }
        ULONG STDMETHODCALLTYPE Release() override {
            if (--ref_count == 0) {
                delete this;
                return 0;
            }
            return ref_count;
        }
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,
                                                 void **ppv) override {
            if (riid == IID_IUnknown || riid == IID_IDropTarget) {
                *ppv = static_cast<::IDropTarget *>(this);
                AddRef();
                return S_OK;
            }
            *ppv = nullptr;
            return E_NOINTERFACE;
        }
        HRESULT STDMETHODCALLTYPE DragEnter(IDataObject *data, DWORD key_state,
                                            POINTL pt, DWORD *effect) override {
            if (has_files(data)) {
                dragging = true;
                begin_imgui_extern_drag(data);
                *effect = DROPEFFECT_COPY;
            } else {
                *effect = DROPEFFECT_NONE;
            }
            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE DragOver(DWORD key_state, POINTL pt,
                                           DWORD *effect) override {
            if (dragging) {
                update_imgui_extern_drag(pt);
                *effect = DROPEFFECT_COPY;
            } else {
                *effect = DROPEFFECT_NONE;
            }
            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE DragLeave() override {
            dragging = false;
            end_imgui_extern_drag();
            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE Drop(IDataObject *data, DWORD key_state,
                                       POINTL pt, DWORD *effect) override {
            dragging = false;
            deliver_imgui_extern_drag(data, pt);
            *effect = DROPEFFECT_COPY;
            return S_OK;
        }

    private:
        bool has_files(IDataObject *data) {
            FORMATETC fmt = {CF_HDROP, nullptr, DVASPECT_CONTENT, -1,
                             TYMED_HGLOBAL};
            return data->QueryGetData(&fmt) == S_OK;
        }

        std::vector<std::string> extract_paths(IDataObject *data) {
            FORMATETC fmt = {CF_HDROP, nullptr, DVASPECT_CONTENT, -1,
                             TYMED_HGLOBAL};
            STGMEDIUM medium = {};
            std::vector<std::string> paths;

            if (FAILED(data->GetData(&fmt, &medium))) return paths;

            HDROP hdrop = (HDROP)GlobalLock(medium.hGlobal);
            if (!hdrop) {
                ReleaseStgMedium(&medium);
                return paths;
            }

            u32 count = DragQueryFileA(hdrop, 0xFFFFFFFF, nullptr, 0);
            for (u32 i = 0; i < count; i++) {
                char buf[MAX_PATH];
                DragQueryFileA(hdrop, i, buf, MAX_PATH);
                paths.push_back(buf);
            }

            GlobalUnlock(medium.hGlobal);
            ReleaseStgMedium(&medium);
            return paths;
        }

        void begin_imgui_extern_drag(IDataObject *data) {
            cached_paths = extract_paths(data);

            ImGuiContext *ctx = ImGui::GetCurrentContext();
            ctx->DragDropSourceFlags = ImGuiDragDropFlags_SourceExtern;
            ctx->DragDropMouseButton = -1;
            ctx->DragDropSourceFrameCount = ctx->FrameCount - 1;
        }

        void update_imgui_extern_drag(POINTL pt) {
            pending_pt = pt;
            has_pending = true;
        }

        void end_imgui_extern_drag() {
            ImGuiContext *ctx = ImGui::GetCurrentContext();
            ctx->DragDropActive = false;
            cached_paths.clear();
            has_pending = false;
        }

        void deliver_imgui_extern_drag(IDataObject *data, POINTL pt) {
            cached_paths = extract_paths(data);
            drop_delivered = true;
            pending_pt = pt;
        }

    public:
        void feed_gui(ImGuiContext *ctx) {
            if (!dragging && !drop_delivered) return;

            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceExtern)) {
                std::string joined;
                for (auto &p : cached_paths) joined += p + "\n";
                ImGui::SetDragDropPayload("EXTERNAL", joined.c_str(),
                                          joined.size() + 1);
                ImGui::EndDragDropSource();
            }

            if (drop_delivered) {
                drop_delivered = false;
                dragging = false;
            }
        }

        std::vector<std::string> cached_paths;
        POINTL pending_pt = {};
        bool has_pending = false;
        bool drop_delivered = false;
};
}  // namespace Seed

#endif