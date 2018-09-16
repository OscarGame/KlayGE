/**
 * @file SceneNode.cpp
 * @author Minmin Gong
 *
 * @section DESCRIPTION
 *
 * This source file is part of KlayGE
 * For the latest info, see http://www.klayge.org
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * You may alternatively use this source under the terms of
 * the KlayGE Proprietary License (KPL). You can obtained such a license
 * from http://www.klayge.org/licensing/.
 */

#include <KlayGE/KlayGE.hpp>
#include <KFL/CXX17/string_view.hpp>
#include <KlayGE/SceneManager.hpp>
#include <KlayGE/Context.hpp>
#include <KFL/Math.hpp>
#include <KlayGE/Renderable.hpp>

#include <boost/assert.hpp>

#include <KlayGE/SceneNode.hpp>

namespace KlayGE
{
	SceneNode::SceneNode(uint32_t attrib)
		: attrib_(attrib), parent_(nullptr),
			model_(float4x4::Identity()), abs_model_(float4x4::Identity()),
			pos_aabb_dirty_(true), visible_mark_(BO_No)
	{
		if (!(attrib & SOA_Overlay) && (attrib & (SOA_Cullable | SOA_Moveable)))
		{
			pos_aabb_os_ = MakeUniquePtr<AABBox>();
			pos_aabb_ws_ = MakeUniquePtr<AABBox>();
		}
	}

	SceneNode::SceneNode(std::wstring_view name, uint32_t attrib)
		: SceneNode(attrib)
	{
		name_ = std::wstring(name);
	}

	SceneNode::SceneNode(RenderablePtr const & renderable, uint32_t attrib)
		: SceneNode(attrib)
	{
		this->AddRenderable(renderable);
		this->OnAttachRenderable(false);
	}

	SceneNode::SceneNode(RenderablePtr const & renderable, std::wstring_view name, uint32_t attrib)
		: SceneNode(renderable, attrib)
	{
		name_ = std::wstring(name);
	}

	SceneNode::~SceneNode()
	{
	}

	std::wstring_view SceneNode::Name()
	{
		return name_;
	}

	void SceneNode::Name(std::wstring_view name)
	{
		name_ = std::wstring(name);
	}

	SceneNode* SceneNode::FindFirstNode(std::wstring_view name)
	{
		SceneNode* ret = nullptr;
		if (name_ == name)
		{
			ret = this;
		}
		else
		{
			for (auto const & child : children_)
			{
				ret = child->FindFirstNode(name);
				if (ret != nullptr)
				{
					break;
				}
			}
		}

		return ret;
	}

	std::vector<SceneNode*> SceneNode::FindAllNode(std::wstring_view name)
	{
		std::vector<SceneNode*> ret;
		this->FindAllNode(ret, name);
		return ret;
	}

	void SceneNode::FindAllNode(std::vector<SceneNode*>& nodes, std::wstring_view name)
	{
		if (name_ == name)
		{
			nodes.push_back(this);
		}

		for (auto const & child : children_)
		{
			child->FindAllNode(nodes, name);
		}
	}

	bool SceneNode::IsNodeInSubTree(SceneNode const * node)
	{
		if (node == this)
		{
			return true;
		}
		else
		{
			for (auto const & child : children_)
			{
				if (child->IsNodeInSubTree(node))
				{
					return true;
				}
			}
		}

		return false;
	}

	SceneNode* SceneNode::Parent() const
	{
		return parent_;
	}

	void SceneNode::Parent(SceneNode* so)
	{
		parent_ = so;
	}

	std::vector<SceneNodePtr> const & SceneNode::Children() const
	{
		return children_;
	}

	SceneNodePtr const & SceneNode::GetChildNode(uint32_t i) const
	{
		BOOST_ASSERT(i < children_.size());
		return children_[i];
	}

	void SceneNode::AddChild(SceneNodePtr const & node)
	{
		pos_aabb_dirty_ = true;
		children_.push_back(node);
	}

	void SceneNode::RemoveChild(SceneNodePtr const & node)
	{
		auto iter = std::find(children_.begin(), children_.end(), node);
		if (iter != children_.end())
		{
			pos_aabb_dirty_ = true;
			children_.erase(iter);
		}
	}

	void SceneNode::ClearChildren()
	{
		pos_aabb_dirty_ = true;
		children_.clear();
	}

	uint32_t SceneNode::NumRenderables() const
	{
		return static_cast<uint32_t>(renderables_.size());
	}

	RenderablePtr const & SceneNode::GetRenderable() const
	{
		return this->GetRenderable(0);
	}

	RenderablePtr const & SceneNode::GetRenderable(uint32_t i) const
	{
		return renderables_[i];
	}

	void SceneNode::AddRenderable(RenderablePtr const & renderable)
	{
		renderables_.push_back(renderable);
		renderables_hw_res_ready_.push_back(false);
		pos_aabb_dirty_ = true;
	}

	void SceneNode::DelRenderable(RenderablePtr const & renderable)
	{
		auto iter = std::find(renderables_.begin(), renderables_.end(), renderable);
		if (iter != renderables_.end())
		{
			renderables_.erase(iter);
			renderables_hw_res_ready_.erase(renderables_hw_res_ready_.begin() + (iter - renderables_.begin()));
			pos_aabb_dirty_ = true;
		}
	}

	void SceneNode::ModelMatrix(float4x4 const & mat)
	{
		model_ = mat;
	}

	float4x4 const & SceneNode::ModelMatrix() const
	{
		return model_;
	}

	float4x4 const & SceneNode::AbsModelMatrix() const
	{
		return abs_model_;
	}

	AABBox const & SceneNode::PosBoundWS() const
	{
		return *pos_aabb_ws_;
	}

	void SceneNode::UpdateAbsModelMatrix()
	{
		if (parent_)
		{
			abs_model_ = parent_->ModelMatrix() * model_;
		}
		else
		{
			abs_model_ = model_;
		}

		if (!renderables_.empty())
		{
			if (pos_aabb_ws_)
			{
				this->UpdatePosBound();
				*pos_aabb_ws_ = MathLib::transform_aabb(*pos_aabb_os_, abs_model_);
			}

			for (auto const & renderable : renderables_)
			{
				renderable->ModelMatrix(abs_model_);
			}
		}
	}

	void SceneNode::VisibleMark(BoundOverlap vm)
	{
		visible_mark_ = vm;
	}

	BoundOverlap SceneNode::VisibleMark() const
	{
		return visible_mark_;
	}

	void SceneNode::BindSubThreadUpdateFunc(std::function<void(SceneNode&, float, float)> const & update_func)
	{
		sub_thread_update_func_ = update_func;
	}

	void SceneNode::BindMainThreadUpdateFunc(std::function<void(SceneNode&, float, float)> const & update_func)
	{
		main_thread_update_func_ = update_func;
	}

	void SceneNode::SubThreadUpdate(float app_time, float elapsed_time)
	{
		if (sub_thread_update_func_)
		{
			sub_thread_update_func_(*this, app_time, elapsed_time);
		}
	}

	bool SceneNode::MainThreadUpdate(float app_time, float elapsed_time)
	{
		bool refreshed = false;
		for (size_t i = 0; i < renderables_.size(); ++ i)
		{
			if (renderables_[i] && !renderables_hw_res_ready_[i] && renderables_[i]->HWResourceReady())
			{
				renderables_hw_res_ready_[i] = true;
				refreshed = true;
			}
		}

		if (refreshed)
		{
			this->OnAttachRenderable(false);
			this->UpdateAbsModelMatrix();
		}

		if (main_thread_update_func_)
		{
			main_thread_update_func_(*this, app_time, elapsed_time);
		}

		return refreshed;
	}

	void SceneNode::AddToSceneManager()
	{
		Context::Instance().SceneManagerInstance().AddSceneNode(this->shared_from_this());
		for (auto const & child : children_)
		{
			child->AddToSceneManager();
		}
	}

	void SceneNode::AddToSceneManagerLocked()
	{
		Context::Instance().SceneManagerInstance().AddSceneNodeLocked(this->shared_from_this());
		for (auto const & child : children_)
		{
			child->AddToSceneManagerLocked();
		}
	}

	void SceneNode::DelFromSceneManager()
	{
		for (auto const & child : children_)
		{
			child->DelFromSceneManager();
		}
		Context::Instance().SceneManagerInstance().DelSceneNode(this->shared_from_this());
	}

	void SceneNode::DelFromSceneManagerLocked()
	{
		for (auto const & child : children_)
		{
			child->DelFromSceneManagerLocked();
		}
		Context::Instance().SceneManagerInstance().DelSceneNodeLocked(this->shared_from_this());
	}

	uint32_t SceneNode::Attrib() const
	{
		return attrib_;
	}

	bool SceneNode::Visible() const
	{
		return (0 == (attrib_ & SOA_Invisible));
	}

	void SceneNode::Visible(bool vis)
	{
		if (vis)
		{
			attrib_ &= ~SOA_Invisible;
		}
		else
		{
			attrib_ |= SOA_Invisible;
		}

		for (auto const & child : children_)
		{
			child->Visible(vis);
		}
	}

	std::vector<VertexElement> const & SceneNode::InstanceFormat() const
	{
		return instance_format_;
	}

	void const * SceneNode::InstanceData() const
	{
		return nullptr;
	}

	void SceneNode::SelectMode(bool select_mode)
	{
		for (auto const & renderable : renderables_)
		{
			renderable->SelectMode(select_mode);
		}
	}

	void SceneNode::ObjectID(uint32_t id)
	{
		for (auto const & renderable : renderables_)
		{
			renderable->ObjectID(id);
		}
	}

	bool SceneNode::SelectMode() const
	{
		if (renderables_[0])
		{
			return renderables_[0]->SelectMode();
		}
		else
		{
			return false;
		}
	}

	void SceneNode::Pass(PassType type)
	{
		for (auto const & renderable : renderables_)
		{
			renderable->Pass(type);
		}

		if (attrib_ & SOA_NotCastShadow)
		{
			this->Visible(PC_ShadowMap != GetPassCategory(type));
		}
	}

	bool SceneNode::TransparencyBackFace() const
	{
		if (renderables_[0])
		{
			return renderables_[0]->TransparencyBackFace();
		}
		else
		{
			return false;
		}
	}

	bool SceneNode::TransparencyFrontFace() const
	{
		if (renderables_[0])
		{
			return renderables_[0]->TransparencyFrontFace();
		}
		else
		{
			return false;
		}
	}

	bool SceneNode::SSS() const
	{
		if (renderables_[0])
		{
			return renderables_[0]->SSS();
		}
		else
		{
			return false;
		}
	}

	bool SceneNode::Reflection() const
	{
		if (renderables_[0])
		{
			return renderables_[0]->Reflection();
		}
		else
		{
			return false;
		}
	}

	bool SceneNode::SimpleForward() const
	{
		if (renderables_[0])
		{
			return renderables_[0]->SimpleForward();
		}
		else
		{
			return false;
		}
	}

	bool SceneNode::VDM() const
	{
		if (renderables_[0])
		{
			return renderables_[0]->VDM();
		}
		else
		{
			return false;
		}
	}

	void SceneNode::OnAttachRenderable(bool add_to_scene)
	{
		for (auto const & renderable : renderables_)
		{
			if (renderable && (renderable->NumSubrenderables() > 0))
			{
				size_t const base = children_.size();
				children_.resize(base + renderable->NumSubrenderables());
				for (uint32_t i = 0; i < renderable->NumSubrenderables(); ++ i)
				{
					auto child = MakeSharedPtr<SceneNode>(renderable->Subrenderable(i), attrib_);
					child->Parent(this);
					children_[base + i] = child;

					if (add_to_scene)
					{
						child->AddToSceneManagerLocked();
					}
				}
			}
		}
	}

	void SceneNode::UpdatePosBound()
	{
		if (pos_aabb_dirty_)
		{
			if (pos_aabb_os_)
			{
				*pos_aabb_os_ = renderables_[0]->PosBound();
				for (size_t i = 1; i < renderables_.size(); ++ i)
				{
					*pos_aabb_os_ |= renderables_[i]->PosBound();
				}

				for (size_t i = 0; i < children_.size(); ++ i)
				{
					if (children_[i]->pos_aabb_os_)
					{
						children_[i]->UpdatePosBound();
						*pos_aabb_os_ |= *children_[i]->pos_aabb_os_;
					}
				}
			}

			pos_aabb_dirty_ = false;
		}
	}
}
