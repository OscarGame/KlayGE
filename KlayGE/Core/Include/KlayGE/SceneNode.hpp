/**
 * @file SceneNode.hpp
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

#ifndef KLAYGE_CORE_SCENE_NODE_HPP
#define KLAYGE_CORE_SCENE_NODE_HPP

#pragma once

#include <KlayGE/PreDeclare.hpp>
#include <KlayGE/RenderLayout.hpp>
#include <KlayGE/Renderable.hpp>

namespace KlayGE
{
	class KLAYGE_CORE_API SceneNode : boost::noncopyable, public std::enable_shared_from_this<SceneNode>
	{
	public:
		enum SOAttrib
		{
			SOA_Cullable = 1UL << 0,
			SOA_Overlay = 1UL << 1,
			SOA_Moveable = 1UL << 2,
			SOA_Invisible = 1UL << 3,
			SOA_NotCastShadow = 1UL << 4,
			SOA_SSS = 1UL << 5
		};

	public:
		explicit SceneNode(uint32_t attrib);
		SceneNode(std::wstring_view name, uint32_t attrib);
		SceneNode(RenderablePtr const & renderable, uint32_t attrib);
		SceneNode(RenderablePtr const & renderable, std::wstring_view name, uint32_t attrib);
		virtual ~SceneNode();

		std::wstring_view Name();
		void Name(std::wstring_view name);

		SceneNode* FindFirstNode(std::wstring_view name);
		std::vector<SceneNode*> FindAllNode(std::wstring_view name);

		bool IsNodeInSubTree(SceneNode const * node);

		SceneNode* Parent() const;
		void Parent(SceneNode* node);

		std::vector<SceneNodePtr> const & Children() const;
		SceneNodePtr const & GetChildNode(uint32_t i) const;
		void AddChild(SceneNodePtr const & node);
		void RemoveChild(SceneNodePtr const & node);
		void ClearChildren();

		uint32_t NumRenderables() const;
		RenderablePtr const & GetRenderable() const;
		RenderablePtr const & GetRenderable(uint32_t i) const;

		void AddRenderable(RenderablePtr const & renderable);
		void DelRenderable(RenderablePtr const & renderable);

		virtual void ModelMatrix(float4x4 const & mat);
		virtual float4x4 const & ModelMatrix() const;
		virtual float4x4 const & AbsModelMatrix() const;
		virtual AABBox const & PosBoundWS() const;
		void UpdateAbsModelMatrix();
		void VisibleMark(BoundOverlap vm);
		BoundOverlap VisibleMark() const;

		virtual void OnAttachRenderable(bool add_to_scene);

		virtual void AddToSceneManager();
		virtual void DelFromSceneManager();

		void BindSubThreadUpdateFunc(std::function<void(SceneNode&, float, float)> const & update_func);
		void BindMainThreadUpdateFunc(std::function<void(SceneNode&, float, float)> const & update_func);

		virtual void SubThreadUpdate(float app_time, float elapsed_time);
		virtual bool MainThreadUpdate(float app_time, float elapsed_time);

		uint32_t Attrib() const;
		bool Visible() const;
		void Visible(bool vis);

		std::vector<VertexElement> const & InstanceFormat() const;
		virtual void const * InstanceData() const;

		// For select mode
		virtual void ObjectID(uint32_t id);
		virtual void SelectMode(bool select_mode);
		bool SelectMode() const;

		// For deferred only
		virtual void Pass(PassType type);

		bool TransparencyBackFace() const;
		bool TransparencyFrontFace() const;
		bool SSS() const;
		bool Reflection() const;
		bool SimpleForward() const;
		bool VDM() const;

	private:
		void FindAllNode(std::vector<SceneNode*>& nodes, std::wstring_view name);

		virtual void AddToSceneManagerLocked();
		virtual void DelFromSceneManagerLocked();

		void UpdatePosBound();

	protected:
		std::wstring name_;

		uint32_t attrib_;

		SceneNode* parent_;
		std::vector<SceneNodePtr> children_;

		std::vector<RenderablePtr> renderables_;
		std::vector<bool> renderables_hw_res_ready_;
		std::vector<VertexElement> instance_format_;

		float4x4 model_;
		float4x4 abs_model_;
		std::unique_ptr<AABBox> pos_aabb_os_;
		std::unique_ptr<AABBox> pos_aabb_ws_;
		bool pos_aabb_dirty_;
		BoundOverlap visible_mark_;

		std::function<void(SceneNode&, float, float)> sub_thread_update_func_;
		std::function<void(SceneNode&, float, float)> main_thread_update_func_;
	};
}

#endif		// KLAYGE_CORE_SCENE_NODE_HPP
