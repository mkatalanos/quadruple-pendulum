#include <box2d/box2d.h>
#include <format>
#include <functional>
#include <raylib.h>
#include <string>
#include <vector>

constexpr int WINDOW_WIDTH = 1920, WINDOW_HEIGHT = 1080;
// constexpr int WINDOW_WIDTH = 1280, WINDOW_HEIGHT = 720;
constexpr float lengthUnitsPerMeter = 128.0f;

struct Physics {
  b2BodyId id = b2_nullBodyId;
  b2ShapeId shapeId = b2_nullShapeId;
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

    m_tracked_entities = std::vector<Entity>(5);
  }
  ~EntityManager() { b2DestroyWorld(m_worldId); }

  Entity &CreateBox(Vector2 position, Vector2 size);
  Entity &CreateDummy(Vector2 position);
  Entity &CreateGround(Vector2 position, Vector2 size);
  Entity &CreateMass(Vector2 position, float size, Color color);

  void MousePressed(Vector2 pos);
  void MouseReleased();

  void UpdateSimulation(float deltaTime, int subdivbisions = 4) {
    b2World_Step(m_worldId, deltaTime, subdivbisions);
  }

  b2WorldId GetWorld() { return m_worldId; }

  void RegisterEntity(Entity &&entity) {
    m_tracked_entities.push_back(std::move(entity));
  }

  void RenderAll() {
    for (auto &e : m_tracked_entities) {
      e.Render();
    }

    if (b2Joint_IsValid(m_mouseJointId)) {

      DrawText("Mouse Grab On", 0, 20, 20, GREEN);

      auto target = b2MouseJoint_GetTarget(m_mouseJointId);
      DrawCircle(target.x, target.y, 5, BLACK);
    } else
      DrawText("Mouse Grab off", 0, 20, 20, GREEN);
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

Entity &EntityManager::CreateMass(Vector2 position, float size, Color color) {
  // Create Body
  auto bodyDef = b2DefaultBodyDef();
  bodyDef.type = b2_dynamicBody;
  bodyDef.position = b2Vec2(position.x, position.y);
  b2BodyId bodyId = b2CreateBody(m_worldId, &bodyDef);

  // Add Shape
  auto shapeDef = b2DefaultShapeDef();
  shapeDef.density = 1;

  b2Circle circle{{0, 0}, size};
  b2ShapeId shapeId = b2CreateCircleShape(bodyId, &shapeDef, &circle);

  // Prepare entity
  Entity entity;
  entity.physics.id = bodyId;
  entity.physics.shapeId = shapeId;

  // Attach Rendering function
  entity.render_ = [bodyId, size, color] {
    // std::cout << "Rendering Box: " << bodyId.index1
    //           << " with shape id: " << shapeId.index1 << std::endl;

    b2Vec2 p = b2Body_GetWorldPoint(bodyId, {0, 0});
    DrawCircle(p.x, p.y, size, color);
    // DrawRectanglePro(rec, Vector2{0, 0}, angle, BLUE);
  };

  m_tracked_entities.push_back(std::move(entity));
  return m_tracked_entities.back();
}

Entity &EntityManager::CreateDummy(Vector2 position) {
  auto bodyDef = b2DefaultBodyDef();
  bodyDef.position = b2Vec2{position.x, position.y};
  b2BodyId bodyId = b2CreateBody(m_worldId, &bodyDef);

  Entity entity;
  entity.physics.id = bodyId;

  entity.render_ = [bodyId] {
    b2Vec2 p = b2Body_GetWorldPoint(bodyId, {-5, -5});
    DrawRectangle(p.x, p.y, 10, 10, BLACK);
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
  // em.CreateGround({int{WINDOW_WIDTH / 2}, WINDOW_HEIGHT - 20},
  //                 {WINDOW_WIDTH, 40});
  // em.CreateGround({int{WINDOW_WIDTH / 2}, 5}, {WINDOW_WIDTH, 10});
  // em.CreateGround({5, int{WINDOW_HEIGHT / 2}}, {10, WINDOW_HEIGHT});
  // em.CreateGround({int{WINDOW_WIDTH - 5}, int{WINDOW_HEIGHT / 2}},
  //                 {10, WINDOW_HEIGHT});

  float a1 = 0;
  float l1 = 200;
  float a2 = 0;
  float l2 = 200;
  float a3 = 0;
  float l3 = 200;
  float a4 = 0;
  float l4 = 200;
  Vector2 anchorPos{int{WINDOW_WIDTH / 2}, 80};
  Vector2 mass1InitPos{anchorPos.x + l1 * cos(DEG2RAD * a1),
                       anchorPos.y + l1 * sin(DEG2RAD * a1)};
  Vector2 mass2InitPos{mass1InitPos.x + l2 * cos(DEG2RAD * a2),
                       mass1InitPos.y + l2 * sin(DEG2RAD * a2)};
  Vector2 mass3InitPos{mass2InitPos.x + l3 * cos(DEG2RAD * a3),
                       mass2InitPos.y + l3 * sin(DEG2RAD * a3)};
  Vector2 mass4InitPos{mass3InitPos.x + l4 * cos(DEG2RAD * a4),
                       mass3InitPos.y + l4 * sin(DEG2RAD * a4)};

  auto &anchor = em.CreateDummy(anchorPos);
  auto &mass1 = em.CreateMass(mass1InitPos, 40, PINK);
  auto &mass2 = em.CreateMass(mass2InitPos, 40, BLUE);
  auto &mass3 = em.CreateMass(mass3InitPos, 40, RED);
  auto &mass4 = em.CreateMass(mass4InitPos, 40, GREEN);

  {
    auto jointDef = b2DefaultDistanceJointDef();
    jointDef.bodyIdA = anchor.physics.id;
    jointDef.bodyIdB = mass1.physics.id;
    jointDef.length = l1;
    b2CreateDistanceJoint(em.GetWorld(), &jointDef);
  }
  {
    auto jointDef = b2DefaultDistanceJointDef();
    jointDef.bodyIdA = mass1.physics.id;
    jointDef.bodyIdB = mass2.physics.id;
    jointDef.length = l2;
    b2CreateDistanceJoint(em.GetWorld(), &jointDef);
  }
  {
    auto jointDef = b2DefaultDistanceJointDef();
    jointDef.bodyIdA = mass2.physics.id;
    jointDef.bodyIdB = mass3.physics.id;
    jointDef.length = l3;
    b2CreateDistanceJoint(em.GetWorld(), &jointDef);
  }
  {
    auto jointDef = b2DefaultDistanceJointDef();
    jointDef.bodyIdA = mass3.physics.id;
    jointDef.bodyIdB = mass4.physics.id;
    jointDef.length = l4;
    b2CreateDistanceJoint(em.GetWorld(), &jointDef);
  }

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

    // Render lines
    {
      b2Vec2 pos1 = b2Body_GetWorldPoint(mass1.physics.id, {0, 0});
      b2Vec2 pos2 = b2Body_GetWorldPoint(mass2.physics.id, {0, 0});
      b2Vec2 pos3 = b2Body_GetWorldPoint(mass3.physics.id, {0, 0});
      b2Vec2 pos4 = b2Body_GetWorldPoint(mass4.physics.id, {0, 0});

      DrawLineEx({anchorPos.x, anchorPos.y}, {pos1.x, pos1.y}, 4, LIGHTGRAY);
      DrawLineEx({pos1.x, pos1.y}, {pos2.x, pos2.y}, 4, LIGHTGRAY);
      DrawLineEx({pos2.x, pos2.y}, {pos3.x, pos3.y}, 4, LIGHTGRAY);
      DrawLineEx({pos3.x, pos3.y}, {pos4.x, pos4.y}, 4, LIGHTGRAY);
    }
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
