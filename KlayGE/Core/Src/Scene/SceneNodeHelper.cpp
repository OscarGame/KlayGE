// SceneObjectHelper.cpp
// KlayGE һЩ���õĿ���Ⱦ���� ʵ���ļ�
// Ver 3.12.0
// ��Ȩ����(C) ������, 2005-2011
// Homepage: http://www.klayge.org
//
// 3.10.0
// SceneObjectSkyBox��SceneObjectHDRSkyBox������Technique() (2010.1.4)
//
// 3.9.0
// ������SceneObjectHDRSkyBox (2009.5.4)
//
// 2.7.1
// ������RenderableHelper���� (2005.7.10)
//
// 2.6.0
// ������RenderableSkyBox (2005.5.26)
//
// 2.5.0
// ������RenderablePoint��RenderableLine��RenderableTriangle (2005.4.13)
//
// 2.4.0
// ���ν��� (2005.3.22)
//
// �޸ļ�¼
//////////////////////////////////////////////////////////////////////////////////

#include <KlayGE/KlayGE.hpp>
#include <KFL/ErrorHandling.hpp>
#include <KFL/Util.hpp>
#include <KFL/Math.hpp>
#include <KlayGE/RenderableHelper.hpp>
#include <KlayGE/SkyBox.hpp>
#include <KlayGE/Mesh.hpp>
#include <KlayGE/Light.hpp>
#include <KlayGE/Camera.hpp>

#include <boost/assert.hpp>

#include <KlayGE/SceneNodeHelper.hpp>

namespace KlayGE
{
	SceneObjectSkyBox::SceneObjectSkyBox(uint32_t attrib)
		: SceneNode(MakeSharedPtr<RenderableSkyBox>(), attrib | SOA_NotCastShadow)
	{
	}

	void SceneObjectSkyBox::Technique(RenderEffectPtr const & effect, RenderTechnique* tech)
	{
		checked_pointer_cast<RenderableSkyBox>(renderables_[0])->Technique(effect, tech);
	}

	void SceneObjectSkyBox::CubeMap(TexturePtr const & cube)
	{
		checked_pointer_cast<RenderableSkyBox>(renderables_[0])->CubeMap(cube);
	}

	void SceneObjectSkyBox::CompressedCubeMap(TexturePtr const & y_cube, TexturePtr const & c_cube)
	{
		checked_pointer_cast<RenderableSkyBox>(renderables_[0])->CompressedCubeMap(y_cube, c_cube);
	}


	SceneObjectLightSourceProxy::SceneObjectLightSourceProxy(LightSourcePtr const & light)
		: SceneObjectLightSourceProxy(light, CreateMeshFactory<RenderableLightSourceProxy>())
	{
	}

	SceneObjectLightSourceProxy::SceneObjectLightSourceProxy(LightSourcePtr const & light, RenderModelPtr const & light_model)
		: SceneNode(light_model, SOA_Cullable | SOA_Moveable | SOA_NotCastShadow),
			light_(light)
	{
		model_scaling_ = float4x4::Identity();

		children_.resize(light_model->NumSubrenderables());
		for (uint32_t i = 0; i < light_model->NumSubrenderables(); ++ i)
		{
			checked_pointer_cast<RenderableLightSourceProxy>(light_model->Subrenderable(i))->AttachLightSrc(light);

			auto child = MakeSharedPtr<SceneNode>(light_model->Subrenderable(i), attrib_);
			child->Parent(this);
			children_[i] = child;
		}
	}

	SceneObjectLightSourceProxy::SceneObjectLightSourceProxy(LightSourcePtr const & light,
			std::function<StaticMeshPtr(RenderModelPtr const &, std::wstring const &)> CreateMeshFactoryFunc)
		: SceneObjectLightSourceProxy(light, this->LoadModel(light, CreateMeshFactoryFunc))
	{
	}

	bool SceneObjectLightSourceProxy::MainThreadUpdate(float /*app_time*/, float /*elapsed_time*/)
	{
		model_ = model_scaling_ * MathLib::to_matrix(light_->Rotation()) * MathLib::translation(light_->Position());
		if (LightSource::LT_Spot == light_->Type())
		{
			float radius = light_->CosOuterInner().w();
			model_ = MathLib::scaling(radius, radius, 1.0f) * model_;
		}

		RenderModelPtr light_model = checked_pointer_cast<RenderModel>(renderables_[0]);
		for (uint32_t i = 0; i < light_model->NumSubrenderables(); ++ i)
		{
			RenderableLightSourceProxyPtr light_mesh = checked_pointer_cast<RenderableLightSourceProxy>(light_model->Subrenderable(i));
			light_mesh->Update();
		}

		return false;
	}

	void SceneObjectLightSourceProxy::Scaling(float x, float y, float z)
	{
		model_scaling_ = MathLib::scaling(x, y, z);
	}

	void SceneObjectLightSourceProxy::Scaling(float3 const & s)
	{
		model_scaling_ = MathLib::scaling(s);
	}

	RenderModelPtr SceneObjectLightSourceProxy::LoadModel(LightSourcePtr const & light,
		std::function<StaticMeshPtr(RenderModelPtr const &, std::wstring const &)> CreateMeshFactoryFunc)
	{
		std::string mesh_name;
		switch (light->Type())
		{
		case LightSource::LT_Ambient:
			mesh_name = "ambient_light_proxy.meshml";
			break;

		case LightSource::LT_Point:
		case LightSource::LT_SphereArea:
			mesh_name = "point_light_proxy.meshml";
			break;

		case LightSource::LT_Directional:
			mesh_name = "directional_light_proxy.meshml";
			break;

		case LightSource::LT_Spot:
			mesh_name = "spot_light_proxy.meshml";
			break;

		case LightSource::LT_TubeArea:
			mesh_name = "tube_light_proxy.meshml";
			break;

		default:
			KFL_UNREACHABLE("Invalid light type");
		}
		return SyncLoadModel(mesh_name.c_str(), EAH_GPU_Read | EAH_Immutable,
			CreateModelFactory<RenderModel>(), CreateMeshFactoryFunc);
	}


	SceneObjectCameraProxy::SceneObjectCameraProxy(CameraPtr const & camera)
		: SceneObjectCameraProxy(camera, CreateMeshFactory<RenderableCameraProxy>())
	{
	}

	SceneObjectCameraProxy::SceneObjectCameraProxy(CameraPtr const & camera, RenderModelPtr const & camera_model)
		: SceneNode(camera_model, SOA_Cullable | SOA_Moveable | SOA_NotCastShadow),
			camera_(camera)
	{
		model_scaling_ = float4x4::Identity();

		children_.resize(camera_model->NumSubrenderables());
		for (uint32_t i = 0; i < camera_model->NumSubrenderables(); ++ i)
		{
			checked_pointer_cast<RenderableCameraProxy>(camera_model->Subrenderable(i))->AttachCamera(camera);

			auto child = MakeSharedPtr<SceneNode>(camera_model->Subrenderable(i), attrib_);
			child->Parent(this);
			children_[i] = child;
		}
	}

	SceneObjectCameraProxy::SceneObjectCameraProxy(CameraPtr const & camera,
			std::function<StaticMeshPtr(RenderModelPtr const &, std::wstring const &)> CreateMeshFactoryFunc)
		: SceneObjectCameraProxy(camera, this->LoadModel(CreateMeshFactoryFunc))
	{
	}

	void SceneObjectCameraProxy::SubThreadUpdate(float /*app_time*/, float /*elapsed_time*/)
	{
		model_ = model_scaling_ * camera_->InverseViewMatrix();
	}

	void SceneObjectCameraProxy::Scaling(float x, float y, float z)
	{
		model_scaling_ = MathLib::scaling(x, y, z);
	}

	void SceneObjectCameraProxy::Scaling(float3 const & s)
	{
		model_scaling_ = MathLib::scaling(s);
	}

	RenderModelPtr SceneObjectCameraProxy::LoadModel(
		std::function<StaticMeshPtr(RenderModelPtr const &, std::wstring const &)> CreateMeshFactoryFunc)
	{
		return SyncLoadModel("camera_proxy.meshml", EAH_GPU_Read | EAH_Immutable,
			CreateModelFactory<RenderModel>(), CreateMeshFactoryFunc);
	}
}
