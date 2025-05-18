#ifndef _DEPENDENCY_REGISTRY_HPP_
#define _DEPENDENCY_REGISTRY_HPP_

#include <memory>
#include <unordered_map>

enum class DependencyType
{
  project_config,
  camera_manager
};

class DependencyRegistry
{
  std::unordered_map<DependencyType, std::shared_ptr<void>> services;

public:
  template <typename ServiceType>
  void registerService(DependencyType dependencyType, std::shared_ptr<ServiceType> service)
  {
    this->services[dependencyType] = std::static_pointer_cast<void>(service);
  }

  template <typename ServiceType>
  std::shared_ptr<ServiceType> resolve(DependencyType dependencyType)
  {
    auto serviceIT = this->services.find(dependencyType);
    if (serviceIT != this->services.end())
    {
      return std::static_pointer_cast<ServiceType>(serviceIT->second);
    }

    return nullptr;
  }
};

#endif