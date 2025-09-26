#include "body.h"
#include "box2d/id.h"
#include "box2d/types.h"
#include <box2d/box2d.h>
#include <functional>
#include <iostream>
#include <raylib.h>

constexpr int WINDOW_WIDTH = 1920, WINDOW_HEIGHT = 1080;
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
  }

private:
  std::vector<Entity> m_tracked_entities;
  b2WorldId m_worldId;
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
  entity.render_ = [bodyId, shapeId, size] {
    std::cout << "Rendering Ground: " << bodyId.index1
              << " with shape id: " << shapeId.index1 << std::endl;

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
  entity.render_ = [bodyId, shapeId, size] {
    std::cout << "Rendering Box: " << bodyId.index1
              << " with shape id: " << shapeId.index1 << std::endl;

    b2Vec2 p = b2Body_GetWorldPoint(bodyId, b2Vec2(-size.x / 2, -size.y / 2));
    float angle = RAD2DEG * b2Rot_GetAngle(b2Body_GetRotation(bodyId));
    Rectangle rec(p.x, p.y, size.x, size.y);
    DrawRectanglePro(rec, Vector2{0, 0}, angle, BLUE);

    printf("%d:(%f,%f)\n", bodyId.index1, p.x, p.y);
  };

  m_tracked_entities.push_back(std::move(entity));
  return m_tracked_entities.back();
}

int main(void) {
  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Double Pendulum");
  b2SetLengthUnitsPerMeter(lengthUnitsPerMeter);
  SetTargetFPS(60);

  EntityManager em;
  em.CreateGround({WINDOW_WIDTH / 2, WINDOW_HEIGHT - 100}, {WINDOW_WIDTH, 100});
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
