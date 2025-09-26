#include <box2d/box2d.h>
#include <cassert>
#include <format>
#include <functional>
#include <iostream>
#include <raylib.h>
#include <string>

constexpr int WINDOW_WIDTH = 1920, WINDOW_HEIGHT = 1080;
// constexpr int WINDOW_WIDTH = 1280, WINDOW_HEIGHT = 720;
constexpr float lengthUnitsPerMeter = 128.0f;

struct Physics {
  b2BodyId id;
  b2ShapeId shapeId;
};

struct Entity {
  Physics physics;

  void Render();

private:
  std::function<void()> render_ = nullptr;

  friend struct EntityManager;
};

void Entity::Render() {
  if (render_) {
    render_();
  }
}

struct EntityManager {

  EntityManager() {
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity.y = 9.8f * lengthUnitsPerMeter;
    m_worldId = b2CreateWorld(&worldDef);
  }
  ~EntityManager() { b2DestroyWorld(m_worldId); }

  Entity &CreateBox(Vector2 position, Vector2 size);
  Entity &CreateGround(Vector2 position, Vector2 size);

  void MousePressed(Vector2 pos);
  void MouseReleased();

  void UpdateSimulation(float deltaTime, int subdivbisions = 4) {
    b2World_Step(m_worldId, deltaTime, subdivbisions);
  }

  void RegisterEntity(Entity &&entity) {
    m_tracked_entities.push_back(std::move(entity));
  }

  void RenderAll() {
    for (auto &e : m_tracked_entities) {
      e.Render();
    }

    if (b2Joint_IsValid(m_mouseJointId)) {

      DrawText(std::format("Joint {}", b2Joint_GetBodyB(m_mouseJointId).index1)
                   .c_str(),
               0, 20, 20, GREEN);

      auto target = b2MouseJoint_GetTarget(m_mouseJointId);
      DrawCircle(target.x, target.y, 5, BLACK);
    } else
      DrawText("Joint off", 0, 20, 20, GREEN);
  }

private:
  std::vector<Entity> m_tracked_entities;

  b2WorldId m_worldId = b2_nullWorldId;
  b2JointId m_mouseJointId = b2_nullJointId;
  b2BodyId m_groundBodyId = b2_nullBodyId;
};

Entity &EntityManager::CreateGround(Vector2 position, Vector2 size) {
  // Create Body
  auto bodyDef = b2DefaultBodyDef();
  bodyDef.position = b2Vec2(position.x, position.y);
  b2BodyId bodyId = b2CreateBody(m_worldId, &bodyDef);

  // Add Shape
  auto groundBox = b2MakeBox(size.x / 2, size.y / 2);
  auto groundShapeDef = b2DefaultShapeDef();
  b2ShapeId shapeId = b2CreatePolygonShape(bodyId, &groundShapeDef, &groundBox);

  // Prepare entity
  Entity entity;
  entity.physics.id = bodyId;
  entity.physics.shapeId = shapeId;

  // Attach Rendering function
  entity.render_ = [bodyId, size] {
    // std::cout << "Rendering Ground: " << bodyId.index1
    //           << " with shape id: " << shapeId.index1 << std::endl;

    b2Vec2 p = b2Body_GetWorldPoint(bodyId, b2Vec2(-size.x / 2, -size.y / 2));
    float angle = RAD2DEG * b2Rot_GetAngle(b2Body_GetRotation(bodyId));
    Rectangle rec(p.x, p.y, size.x, size.y);
    DrawRectanglePro(rec, Vector2{0, 0}, angle, RED);
  };

  m_tracked_entities.push_back(std::move(entity));
  return m_tracked_entities.back();
}

Entity &EntityManager::CreateBox(Vector2 position, Vector2 size) {
  // Create Body
  auto bodyDef = b2DefaultBodyDef();
  bodyDef.type = b2_dynamicBody;
  bodyDef.position = b2Vec2(position.x, position.y);
  b2BodyId bodyId = b2CreateBody(m_worldId, &bodyDef);

  // Add Shape
  auto dynamicBox = b2MakeBox(size.x / 2, size.y / 2);
  auto shapeDef = b2DefaultShapeDef();
  shapeDef.density = 8;
  shapeDef.material.friction = 0.2;
  b2ShapeId shapeId = b2CreatePolygonShape(bodyId, &shapeDef, &dynamicBox);

  // Prepare entity
  Entity entity;
  entity.physics.id = bodyId;
  entity.physics.shapeId = shapeId;

  // Attach Rendering function
  entity.render_ = [bodyId, size] {
    // std::cout << "Rendering Box: " << bodyId.index1
    //           << " with shape id: " << shapeId.index1 << std::endl;

    b2Vec2 p = b2Body_GetWorldPoint(bodyId, b2Vec2(-size.x / 2, -size.y / 2));
    float angle = RAD2DEG * b2Rot_GetAngle(b2Body_GetRotation(bodyId));
    Rectangle rec(p.x, p.y, size.x, size.y);
    DrawRectanglePro(rec, Vector2{0, 0}, angle, BLUE);
  };

  m_tracked_entities.push_back(std::move(entity));
  return m_tracked_entities.back();
}

void EntityManager::MousePressed(Vector2 pos) {
  b2Vec2 p{pos.x, pos.y};
  if (b2Joint_IsValid(m_mouseJointId)) {
    b2MouseJoint_SetTarget(m_mouseJointId, p);
    return;
  }

  puts("Looking for new");
  // Make a small box
  b2AABB box;
  b2Vec2 d{1, 1};
  box.lowerBound = b2Sub(p, d);
  box.upperBound = b2Add(p, d);

  DrawRectangle(static_cast<int>(box.lowerBound.x),
                static_cast<int>(box.lowerBound.y),
                static_cast<int>(box.upperBound.x - box.lowerBound.x),
                static_cast<int>(box.upperBound.y - box.lowerBound.y), LIME);

  // Query the world for overlapping shapes
  struct QueryContext {
    b2Vec2 point;
    b2BodyId bodyId = b2_nullBodyId;
  };

  QueryContext queryContext{p};

  auto QueryCallback = [](b2ShapeId shapeId, void *context) {
    QueryContext *queryContext = static_cast<QueryContext *>(context);
    b2BodyId bodyId = b2Shape_GetBody(shapeId);
    b2BodyType bodyType = b2Body_GetType(bodyId);
    if (bodyType != b2_dynamicBody) {
      // Continue Query
      return true;
    }

    bool overlap = b2Shape_TestPoint(shapeId, queryContext->point);
    if (overlap) {
      // Found Shape
      queryContext->bodyId = bodyId;
      return false;
    }

    return true;
  };
  b2World_OverlapAABB(m_worldId, box, b2DefaultQueryFilter(), QueryCallback,
                      &queryContext);
  // Check if Query has resulted in a shape
  if (B2_IS_NON_NULL(queryContext.bodyId)) {
    puts("mouse pressed: query found something");

    // Set the joint
    if (B2_IS_NULL(m_groundBodyId)) {
      b2BodyDef bodyDef = b2DefaultBodyDef();
      m_groundBodyId = b2CreateBody(m_worldId, &bodyDef);
    }

    b2MouseJointDef mouseDef = b2DefaultMouseJointDef();
    mouseDef.bodyIdA = m_groundBodyId;
    mouseDef.bodyIdB = queryContext.bodyId;
    mouseDef.target = p;
    mouseDef.hertz = 10.0;
    mouseDef.dampingRatio = 0.7f;
    mouseDef.maxForce = 200.0f * b2Body_GetMass(queryContext.bodyId) *
                        b2Length(b2World_GetGravity(m_worldId));
    m_mouseJointId = b2CreateMouseJoint(m_worldId, &mouseDef);
  }
}
void EntityManager::MouseReleased() {
  if (B2_IS_NON_NULL(m_mouseJointId)) {
    b2DestroyJoint(m_mouseJointId);
    m_mouseJointId = b2_nullJointId;
  }
}

int main(void) {
  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Physics Bruh");
  ToggleFullscreen();
  b2SetLengthUnitsPerMeter(lengthUnitsPerMeter);
  SetTargetFPS(60);

  EntityManager em;
  em.CreateGround({int{WINDOW_WIDTH / 2}, WINDOW_HEIGHT - 20},
                  {WINDOW_WIDTH, 40});
  em.CreateGround({int{WINDOW_WIDTH / 2}, 5}, {WINDOW_WIDTH, 10});
  em.CreateGround({5, int{WINDOW_HEIGHT / 2}}, {10, WINDOW_HEIGHT});
  em.CreateGround({int{WINDOW_WIDTH - 5}, int{WINDOW_HEIGHT / 2}},
                  {10, WINDOW_HEIGHT});
  em.CreateBox({200, 200}, {100, 100});
  em.CreateBox({300, 200}, {100, 100});
  em.CreateBox({400, 200}, {100, 100});
  em.CreateBox({500, 200}, {100, 100});
  em.CreateBox({600, 200}, {100, 100});
  em.CreateBox({200, 230}, {100, 100});
  em.CreateBox({300, 230}, {100, 100});
  em.CreateBox({400, 230}, {100, 100});
  em.CreateBox({500, 230}, {100, 100});
  em.CreateBox({600, 230}, {100, 100});

  bool pause = true;

  while (!WindowShouldClose()) {
    if (IsKeyPressed(KEY_P)) {
      pause = !pause;
    }
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
      em.MousePressed(GetMousePosition());
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
      em.MouseReleased();
    }

    if (pause == false) {
      float deltaTime = GetFrameTime();
      em.UpdateSimulation(deltaTime);
    }

    BeginDrawing();
    ClearBackground(DARKGRAY);

    em.RenderAll();

    // Draw Ground
    // {
    // b2Vec2 p = b2Body_GetWorldPoint(groundId, b2Vec2(-50, -10));
    // float angle = b2Rot_GetAngle(b2Body_GetRotation(groundId));
    // Rectangle rec(p.x, p.y, 100, 20);
    // DrawRectanglePro(rec, Vector2(p.x, p.y), angle, RED);
    // }

    if (pause) {
      DrawText("Pause on", 0, 0, 20, GREEN);
    } else {
      DrawText("Pause off", 0, 0, 20, GREEN);
    }

    EndDrawing();
  }

  CloseWindow();

  return 0;
}
