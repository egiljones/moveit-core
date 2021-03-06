/*********************************************************************
* Software License Agreement (BSD License)
*
*  Copyright (c) 2008, Willow Garage, Inc.
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of the Willow Garage nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*********************************************************************/

/* Author: Ioan Sucan, E. Gil Jones */

#include "planning_models/kinematic_state.h"
#include <ros/console.h>

planning_models::KinematicState::LinkState::LinkState(KinematicState *state, const planning_models::KinematicModel::LinkModel* lm) :
  kinematic_state_(state), link_model_(lm), parent_joint_state_(NULL), parent_link_state_(NULL)
{
  global_link_transform_.setIdentity();
  global_collision_body_transform_.setIdentity();
}

planning_models::KinematicState::LinkState::~LinkState(void)
{
  clearAttachedBodies();
}

void planning_models::KinematicState::LinkState::updateGivenGlobalLinkTransform(const Eigen::Affine3d& transform)
{
  global_link_transform_ = transform;
  global_collision_body_transform_ = global_link_transform_* link_model_->getCollisionOriginTransform();
  updateAttachedBodies();
}

void planning_models::KinematicState::LinkState::computeTransform(void)
{
  if (link_model_->isJointReversed())
    global_link_transform_ = (parent_link_state_ ? parent_link_state_->global_link_transform_ : kinematic_state_->getRootTransform()) * (link_model_->getJointOriginTransform() * parent_joint_state_->getVariableTransform()).inverse();
  else
    global_link_transform_ = (parent_link_state_ ? parent_link_state_->global_link_transform_ : kinematic_state_->getRootTransform()) * link_model_->getJointOriginTransform() * parent_joint_state_->getVariableTransform();
  global_collision_body_transform_ = global_link_transform_ * link_model_->getCollisionOriginTransform();
  
  updateAttachedBodies();
}

void planning_models::KinematicState::LinkState::updateAttachedBodies(void)
{  
  for (std::map<std::string, AttachedBody*>::const_iterator it = attached_body_map_.begin() ; it != attached_body_map_.end() ;  ++it)
    it->second->computeTransform();
}

void planning_models::KinematicState::LinkState::attachBody(const std::string &id,
                                                            const std::vector<shapes::ShapeConstPtr> &shapes,
                                                            const EigenSTL::vector_Affine3d &attach_trans,
                                                            const std::vector<std::string> &touch_links)
{
  AttachedBody *ab = new AttachedBody(this, id, shapes, attach_trans, touch_links);
  attached_body_map_[id] = ab;
  kinematic_state_->attached_body_map_[id] = ab;
  ab->computeTransform();
}

bool planning_models::KinematicState::LinkState::hasAttachedBody(const std::string &id) const
{
  return attached_body_map_.find(id) != attached_body_map_.end();
}

const planning_models::KinematicState::AttachedBody* planning_models::KinematicState::LinkState::getAttachedBody(const std::string &id) const
{
  std::map<std::string, AttachedBody*>::const_iterator it = attached_body_map_.find(id);
  if (it == attached_body_map_.end())
  {
    ROS_ERROR("Attached body '%s' not found on link '%s'", id.c_str(), getName().c_str());
    return NULL;
  }
  else
    return it->second;
}

void planning_models::KinematicState::LinkState::getAttachedBodies(std::vector<const AttachedBody*> &attached_bodies) const
{
  attached_bodies.clear();
  attached_bodies.reserve(attached_body_map_.size());
  for (std::map<std::string, AttachedBody*>::const_iterator it = attached_body_map_.begin() ; it != attached_body_map_.end() ;  ++it)
    attached_bodies.push_back(it->second);
}

bool planning_models::KinematicState::LinkState::clearAttachedBody(const std::string &id)
{  
  std::map<std::string, AttachedBody*>::const_iterator it = attached_body_map_.find(id);
  if (it != attached_body_map_.end())
  {
    delete it->second;
    attached_body_map_.erase(id);
    kinematic_state_->attached_body_map_.erase(id);
    return true;
  }
  return false;
}

void planning_models::KinematicState::LinkState::clearAttachedBodies(void)
{ 
  for (std::map<std::string, AttachedBody*>::const_iterator it = attached_body_map_.begin() ; it != attached_body_map_.end() ;  ++it)
  {
    kinematic_state_->attached_body_map_.erase(it->first);
    delete it->second;
  }
  attached_body_map_.clear();
}


