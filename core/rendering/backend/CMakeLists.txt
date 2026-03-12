target_sources(seed PRIVATE 
    vulkan_backend.cc
    render_backend.cc
    )

if(BUILD_XR)
    target_sources(seed_xr PRIVATE
        xr_vulkan_backend.cc
    )
endif()