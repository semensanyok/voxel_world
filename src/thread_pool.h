// TODO: simple async dispach in draw thread on different stages.
// pseudocode:
//```
//    auto player_tasks = std::async(std::launch::async,
//        [](){
//            input->QueryPlayerInput();
//            player->UpdatePlayerCharacter();
//        });
//
//    auto ai_task = std::async(std::launch::async,
//        [](){
//        //we cant parallelize all the AIs together as they communicate with
//        each other, so no parallel for here. for(AICharacter* AiChar :
//        AICharacters)
//        {
//             AiChar->UpdateAI();
//        }
//        });
//
//    //lets wait until both player and AI asyncs are finished before continuing
//    player_tasks.wait();
//    ai_task.wait();
//
//    //world objects cant be updated in parallel as they affect each other too
//    for(WorldObject* Obj : Objects)
//    {
//        Obj->Update();
//    }
//
//    // particles is standalone AND we can update each particle on its own, so
//    we can combine async with parallel for auto particles_task =
//    std::async(std::launch::async,
//        [](){
//            std::for_each(std::execution::par,
//                ParticleSystems.begin(),
//                ParticleSystems.end(),
//                [](Particle* Part){
//                     Part->UpdateParticles();
//                });
//        });
//
//    // same with animation
//    auto animation_task = std::async(std::launch::async,
//        [](){
//            std::for_each(std::execution::par,
//                AICharacters.begin(),
//                AICharacters.end(),
//                [](AICharacter* AiChar){
//                      AiChar->UpdateAnimation();
//                });
//        });
//
//    //physics can also be updated on its own
//    auto physics_task = std::async(std::launch::async,
//        [](){
//           physicsSystem->UpdatePhysics();
//        });
//
//    //synchronize the 3 tasks
//    particles_task.wait();
//    animation_task.wait();
//    physics_task.wait();
//```
//
//
//
// #ifndef THREAD_POOL_H
// #define THREAD_POOL_H
//
// #include "renderer_context.h"
// #include "vw_constants.h"
// #include <atomic>
// #include <functional>
// #include <thread>
// #include <vector>
// #include <vulkan/vulkan_core.h>
//
// enum class ThreadCapabilities { Transfer, Render, Compute };
//
// // ????
// // enum class TaskType {
// //   Transfer,
// //   Buffer,
// //   Render
// //   // ....... etc. first draft. TODO:
// //   // ???
// //   CullOctreeGatherGeometry
// // };
//
// class ThreadLocals {
//   TransferWorker transfer_worker;
// };
//
// class ThreadPool {
// public:
//   // has all workers except main.
//   std::vector<std::thread> worker_threads;
//
//   void init() {
//     worker_threads.resize(1);
//     auto &worker_thread = worker_threads.at(0);
//     // assign culling and vertex gathering tasks
//     // worker_thread = std::thread();
//   };
// };
// #endif
