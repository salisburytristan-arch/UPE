#pragma once

#include <stdint.h>
#include "upe/upe_broadphase.h"
#include "upe/upe_solver.h"
#include "upe/upe_joint.h"

struct UpeContactPair;

struct UpeWorld;
struct UpeMemoryContext;

void UpeWorldSort_BroadphasePairs(UpeBroadphasePair *pairs,
                                  uint32_t pair_count,
                                  const uint32_t *body_indices,
                                  struct UpeMemoryContext *memory);

void UpeWorldSort_Contacts(struct UpeWorld *world,
                           UpeContactPair *contacts,
                           uint32_t contact_count,
                           struct UpeMemoryContext *memory);

void UpeWorldSort_Constraints(UpeSolverConstraint *constraints,
                              uint32_t constraint_count,
                              struct UpeMemoryContext *memory);

void UpeWorldSort_Joints(UpeJoint *joints,
                         uint32_t joint_count,
                         struct UpeMemoryContext *memory);
