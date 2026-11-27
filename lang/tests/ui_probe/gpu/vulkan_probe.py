#!/usr/bin/env python3
"""P0.1 GPU spike: load the Vulkan loader, create an instance/device and a
pipeline cache against the software adapter (lavapipe), and time init.

No Vulkan headers are installed on this host, so the handful of ABI structs is
declared directly with ctypes. Field order/offsets are the Vulkan 1.1/1.2
canonical ones for x86-64. This is a probe, not a runtime.

  python3 vulkan_probe.py [--icd /usr/share/vulkan/icd.d/lvp_icd.json]

Exit codes: 0 = instance+device created, 2 = loader or device unavailable.
"""
import ctypes as C
import json
import os
import sys
import time

VK_SUCCESS = 0

# VkStructureType
APPLICATION_INFO = 0
INSTANCE_CREATE_INFO = 1
DEVICE_QUEUE_CREATE_INFO = 2
DEVICE_CREATE_INFO = 3
PIPELINE_CACHE_CREATE_INFO = 17

VK_API_VERSION_1_1 = (1 << 22) | (1 << 12)


class VkApplicationInfo(C.Structure):
    _fields_ = [
        ("sType", C.c_uint32),
        ("pNext", C.c_void_p),
        ("pApplicationName", C.c_char_p),
        ("applicationVersion", C.c_uint32),
        ("pEngineName", C.c_char_p),
        ("engineVersion", C.c_uint32),
        ("apiVersion", C.c_uint32),
    ]


class VkInstanceCreateInfo(C.Structure):
    _fields_ = [
        ("sType", C.c_uint32),
        ("pNext", C.c_void_p),
        ("flags", C.c_uint32),
        ("pApplicationInfo", C.POINTER(VkApplicationInfo)),
        ("enabledLayerCount", C.c_uint32),
        ("ppEnabledLayerNames", C.c_void_p),
        ("enabledExtensionCount", C.c_uint32),
        ("ppEnabledExtensionNames", C.c_void_p),
    ]


class VkDeviceQueueCreateInfo(C.Structure):
    _fields_ = [
        ("sType", C.c_uint32),
        ("pNext", C.c_void_p),
        ("flags", C.c_uint32),
        ("queueFamilyIndex", C.c_uint32),
        ("queueCount", C.c_uint32),
        ("pQueuePriorities", C.POINTER(C.c_float)),
    ]


class VkDeviceCreateInfo(C.Structure):
    _fields_ = [
        ("sType", C.c_uint32),
        ("pNext", C.c_void_p),
        ("flags", C.c_uint32),
        ("queueCreateInfoCount", C.c_uint32),
        ("pQueueCreateInfos", C.POINTER(VkDeviceQueueCreateInfo)),
        ("enabledLayerCount", C.c_uint32),
        ("ppEnabledLayerNames", C.c_void_p),
        ("enabledExtensionCount", C.c_uint32),
        ("ppEnabledExtensionNames", C.c_void_p),
        ("pEnabledFeatures", C.c_void_p),
    ]


class VkPipelineCacheCreateInfo(C.Structure):
    _fields_ = [
        ("sType", C.c_uint32),
        ("pNext", C.c_void_p),
        ("flags", C.c_uint32),
        ("initialDataSize", C.c_size_t),
        ("pInitialData", C.c_void_p),
    ]


def now():
    return time.monotonic() * 1000.0


def main(argv):
    icd = None
    for i, a in enumerate(argv):
        if a == "--icd" and i + 1 < len(argv):
            icd = argv[i + 1]
    if icd:
        os.environ["VK_ICD_FILENAMES"] = icd
        os.environ["VK_DRIVER_FILES"] = icd

    result = {
        "icd_env": os.environ.get("VK_DRIVER_FILES") or os.environ.get("VK_ICD_FILENAMES"),
        "loader": None,
        "instance": False,
        "devices": [],
        "device_created": False,
        "pipeline_cache_bytes": None,
        "init_ms": None,
        "notes": [],
    }

    t0 = now()
    try:
        vk = C.CDLL("libvulkan.so.1")
    except OSError as e:
        result["notes"].append(f"loader unavailable: {e}")
        print(json.dumps(result, indent=2))
        return 2
    result["loader"] = "libvulkan.so.1"
    t_loader = now()

    vk.vkCreateInstance.restype = C.c_int
    vk.vkCreateInstance.argtypes = [C.POINTER(VkInstanceCreateInfo), C.c_void_p,
                                    C.POINTER(C.c_void_p)]
    vk.vkEnumeratePhysicalDevices.restype = C.c_int
    vk.vkEnumeratePhysicalDevices.argtypes = [C.c_void_p, C.POINTER(C.c_uint32),
                                              C.POINTER(C.c_void_p)]
    vk.vkGetPhysicalDeviceProperties.restype = None
    vk.vkGetPhysicalDeviceProperties.argtypes = [C.c_void_p, C.c_void_p]
    vk.vkGetPhysicalDeviceQueueFamilyProperties.restype = None
    vk.vkGetPhysicalDeviceQueueFamilyProperties.argtypes = [
        C.c_void_p, C.POINTER(C.c_uint32), C.c_void_p]
    vk.vkCreateDevice.restype = C.c_int
    vk.vkCreateDevice.argtypes = [C.c_void_p, C.POINTER(VkDeviceCreateInfo),
                                  C.c_void_p, C.POINTER(C.c_void_p)]
    vk.vkCreatePipelineCache.restype = C.c_int
    vk.vkCreatePipelineCache.argtypes = [C.c_void_p,
                                         C.POINTER(VkPipelineCacheCreateInfo),
                                         C.c_void_p, C.POINTER(C.c_void_p)]
    vk.vkGetPipelineCacheData.restype = C.c_int
    vk.vkGetPipelineCacheData.argtypes = [C.c_void_p, C.c_void_p,
                                          C.POINTER(C.c_size_t), C.c_void_p]
    vk.vkDestroyPipelineCache.restype = None
    vk.vkDestroyPipelineCache.argtypes = [C.c_void_p, C.c_void_p, C.c_void_p]
    vk.vkDestroyDevice.restype = None
    vk.vkDestroyDevice.argtypes = [C.c_void_p, C.c_void_p]
    vk.vkDestroyInstance.restype = None
    vk.vkDestroyInstance.argtypes = [C.c_void_p, C.c_void_p]

    app = VkApplicationInfo(
        sType=APPLICATION_INFO, pNext=None,
        pApplicationName=b"chemical-ui-p0.1", applicationVersion=1,
        pEngineName=b"probe", engineVersion=1, apiVersion=VK_API_VERSION_1_1)
    ici = VkInstanceCreateInfo(
        sType=INSTANCE_CREATE_INFO, pNext=None, flags=0,
        pApplicationInfo=C.pointer(app), enabledLayerCount=0,
        ppEnabledLayerNames=None, enabledExtensionCount=0,
        ppEnabledExtensionNames=None)

    inst = C.c_void_p()
    rc = vk.vkCreateInstance(C.byref(ici), None, C.byref(inst))
    if rc != VK_SUCCESS:
        result["notes"].append(f"vkCreateInstance rc={rc} (no loader/ICD?)")
        print(json.dumps(result, indent=2))
        return 2
    result["instance"] = True

    count = C.c_uint32(0)
    rc = vk.vkEnumeratePhysicalDevices(inst, C.byref(count), None)
    if rc != VK_SUCCESS or count.value == 0:
        result["notes"].append(f"vkEnumeratePhysicalDevices rc={rc} count={count.value}")
        vk.vkDestroyInstance(inst, None)
        print(json.dumps(result, indent=2))
        return 2

    devs = (C.c_void_p * count.value)()
    vk.vkEnumeratePhysicalDevices(inst, C.byref(count), devs)

    chosen = None
    for i in range(count.value):
        props = C.create_string_buffer(4096)
        vk.vkGetPhysicalDeviceProperties(devs[i], C.addressof(props))
        api = C.cast(C.addressof(props), C.POINTER(C.c_uint32))[0]
        driver = C.cast(C.addressof(props), C.POINTER(C.c_uint32))[1]
        vendor = C.cast(C.addressof(props), C.POINTER(C.c_uint32))[2]
        devid = C.cast(C.addressof(props), C.POINTER(C.c_uint32))[3]
        dtype = C.cast(C.addressof(props), C.POINTER(C.c_uint32))[4]
        name = C.string_at(C.addressof(props) + 20).decode("utf-8", "replace")
        qcount = C.c_uint32(0)
        vk.vkGetPhysicalDeviceQueueFamilyProperties(devs[i], C.byref(qcount), None)
        qprops = C.create_string_buffer(256)
        vk.vkGetPhysicalDeviceQueueFamilyProperties(devs[i], C.byref(qcount),
                                                    C.addressof(qprops))
        qflags = C.cast(C.addressof(qprops), C.POINTER(C.c_uint32))[1]
        info = {
            "index": i,
            "name": name,
            "api": f"{api >> 22}.{(api >> 12) & 0x3ff}.{api & 0xfff}",
            "driver_version": driver,
            "vendor_id": f"0x{vendor:04x}",
            "device_id": f"0x{devid:04x}",
            "device_type": {0: "other", 1: "integrated", 2: "discrete",
                            3: "virtual", 4: "cpu"}.get(dtype, str(dtype)),
            "queue_families": qcount.value,
            "queue0_flags": qflags,
            "graphics_queue": bool(qflags & 0x1),
        }
        result["devices"].append(info)
        if info["graphics_queue"] and chosen is None:
            chosen = (devs[i], info)

    if chosen is None:
        result["notes"].append("no graphics queue family")
        vk.vkDestroyInstance(inst, None)
        print(json.dumps(result, indent=2))
        return 2

    dev, info = chosen
    prio = C.c_float(1.0)
    qci = VkDeviceQueueCreateInfo(
        sType=DEVICE_QUEUE_CREATE_INFO, pNext=None, flags=0,
        queueFamilyIndex=0, queueCount=1, pQueuePriorities=C.pointer(prio))
    dci = VkDeviceCreateInfo(
        sType=DEVICE_CREATE_INFO, pNext=None, flags=0,
        queueCreateInfoCount=1, pQueueCreateInfos=C.pointer(qci),
        enabledLayerCount=0, ppEnabledLayerNames=None,
        enabledExtensionCount=0, ppEnabledExtensionNames=None,
        pEnabledFeatures=None)
    device = C.c_void_p()
    rc = vk.vkCreateDevice(dev, C.byref(dci), None, C.byref(device))
    if rc != VK_SUCCESS:
        result["notes"].append(f"vkCreateDevice rc={rc}")
        vk.vkDestroyInstance(inst, None)
        print(json.dumps(result, indent=2))
        return 2
    result["device_created"] = True
    result["chosen"] = info["name"]
    t_device = now()

    # Pipeline cache: the budget assumes a cache; prove it can be created and
    # serialized for reuse across runs.
    pci = VkPipelineCacheCreateInfo(
        sType=PIPELINE_CACHE_CREATE_INFO, pNext=None, flags=0,
        initialDataSize=0, pInitialData=None)
    cache = C.c_void_p()
    rc = vk.vkCreatePipelineCache(device, C.byref(pci), None, C.byref(cache))
    if rc == VK_SUCCESS:
        size = C.c_size_t(0)
        vk.vkGetPipelineCacheData(device, cache, C.byref(size), None)
        data = C.create_string_buffer(size.value) if size.value else None
        if data is not None:
            vk.vkGetPipelineCacheData(device, cache, C.byref(size), data)
        result["pipeline_cache_bytes"] = int(size.value)
        vk.vkDestroyPipelineCache(device, cache, None)
        result["pipeline_cache_created"] = True
    else:
        result["pipeline_cache_created"] = False
        result["notes"].append(f"vkCreatePipelineCache rc={rc}")

    result["init_ms"] = round(now() - t0, 3)
    result["loader_load_ms"] = round(t_loader - t0, 3)
    result["instance_enum_ms"] = round(t_device - t_loader, 3)
    result["device_cache_ms"] = round(now() - t_device, 3)
    vk.vkDestroyDevice(device, None)
    vk.vkDestroyInstance(inst, None)
    print(json.dumps(result, indent=2))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
