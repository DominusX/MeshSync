#pragma once
#include "MeshUtils/MeshUtils.h"
#include "MeshSync/MeshSync.h"
#include "msbBinder.h"

struct msbSettings;
class msbContext;
namespace bl = blender;


enum class msbNormalSyncMode {
    None,
    PerVertex,
    PerIndex,
};

struct msbSettings
{
    ms::ClientSettings client_settings;
    ms::SceneSettings scene_settings;
    msbNormalSyncMode sync_normals = msbNormalSyncMode::PerIndex;
    bool sync_meshes = true;
    bool sync_uvs = true;
    bool sync_colors = true;
    bool sync_bones = true;
    bool sync_poses = true;
    bool sync_blendshapes = true;
    bool sync_animations = true;
    bool sync_cameras = true;
    bool sync_lights = true;
    bool calc_per_index_normals = true;
    bool sample_animation = true;
    int animation_sps = 5;
};


class msbContext : public std::enable_shared_from_this<msbContext>
{
public:
    struct ObjectRecord
    {
        Object *obj = nullptr; // null if joint
        std::string name;
        std::string path;
        float4x4 mat_local;
        float4x4 mat_global;
        bool alive = true;
    };

    msbContext();
    ~msbContext();
    void setup();

    msbSettings&        getSettings();
    const msbSettings&  getSettings() const;
    ms::ScenePtr        getCurrentScene() const;

    void syncAll();
    void syncUpdated();
    void addObject(Object *obj);

    ms::TransformPtr    addTransform(py::object obj);
    ms::TransformPtr    addTransform_(Object *obj);
    ms::TransformPtr    addTransform_(Object *arm, Bone *obj);
    ms::CameraPtr       addCamera(py::object obj);
    ms::CameraPtr       addCamera_(Object *obj);
    ms::LightPtr        addLight(py::object obj);
    ms::LightPtr        addLight_(Object *obj);
    ms::MeshPtr         addMesh(py::object obj);
    ms::MeshPtr         addMesh_(Object *obj);
    void                addDeleted(const std::string& path);

    ms::MaterialPtr addMaterial(py::object material);
    int getMaterialIndex(const Material *mat);
    void extractTransformData(ms::TransformPtr dst, py::object src);
    void extractTransformData_(ms::TransformPtr dst, Object *obj);
    void extractCameraData(ms::CameraPtr dst, py::object src);
    void extractCameraData_(ms::CameraPtr dst, Object *obj);
    void extractLightData(ms::LightPtr dst, py::object src);
    void extractLightData_(ms::LightPtr dst, Object *obj);
    void extractMeshData(ms::MeshPtr dst, py::object src);
    void extractMeshData_(ms::MeshPtr dst, Object *obj);
    ms::TransformPtr exportArmature(Object *obj);

    bool isSending() const;
    void flushPendingList();
    bool prepare();
    void send();

private:
    ObjectRecord & findOrAddObject(Object *obj);
    ObjectRecord & findOrAddObject(Bone *obj);
    ObjectRecord & findOrAddObject(bPoseChannel *obj);

    ms::TransformPtr findBone(const Object *armature, const Bone *bone);

    void doExtractMeshData(ms::Mesh& mesh, Object *obj);
    void doExtractNonEditMeshData(ms::Mesh& mesh, Object *obj);
    void doExtractEditMeshData(ms::Mesh& mesh, Object *obj);
    template<class T>
    std::shared_ptr<T> getCacheOrCreate(std::vector<std::shared_ptr<T>>& cache);

    msbSettings m_settings;
    std::set<Object*> m_added;
    std::set<Object*> m_pending, m_pending_tmp;
    std::map<const Bone*, ms::TransformPtr> m_bones;
    std::vector<ms::TransformPtr> m_transform_cache;
    std::vector<ms::CameraPtr> m_camera_cache;
    std::vector<ms::LightPtr> m_light_cache;
    std::vector<ms::MeshPtr> m_mesh_cache, m_mesh_send;
    std::vector<std::string> m_deleted;
    ms::ScenePtr m_scene = ms::ScenePtr(new ms::Scene());
    std::map<void*, ObjectRecord> m_records;

    ms::SetMessage m_message;
    std::future<void> m_send_future;

    using task_t = std::function<void()>;
    std::vector<task_t> m_extract_tasks;
    std::mutex m_extract_mutex;
};
using msbContextPtr = std::shared_ptr<msbContext>;