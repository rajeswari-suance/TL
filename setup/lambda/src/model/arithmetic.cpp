
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>

#include "model/arithmetic.hpp"
//BOOST_CLASS_EXPORT(model::ArithmeticUnits::Specs)
BOOST_CLASS_EXPORT(model::ArithmeticUnits)

#include "pat/pat.hpp"

namespace model
{

ArithmeticUnits::ArithmeticUnits(const Specs& specs) :
    specs_(specs)
{
  is_specced_ = true;
  is_evaluated_ = false;
  area_ = specs_.area.Get();
}

ArithmeticUnits::Specs ArithmeticUnits::ParseSpecs(config::CompoundConfigNode setting, uint32_t nElements)
{
  Specs specs;

  // Name.
  std::string name = "__ARITH__";
  setting.lookupValue("name", name);
  specs.name = config::parseName(name);
  specs.level_name = specs.name.Get();
  if (setting.exists("attributes"))
  { // parse v0.2, tree like description
    setting = setting.lookup("attributes");
  }

  // Instances.
  std::uint32_t instances;
  if (!setting.lookupValue("instances", instances))
  {
    if (nElements == 0)
    {
      std::cerr << "instances is a required arithmetic parameter" << std::endl;
      assert(false);
    }
    instances = nElements;
    //std::cout << "ArithUnit: " << specs.name << " size: " << instances << std::endl;
  }

  specs.instances = instances;

  // Word size (in bits).
  std::uint32_t word_bits;
  if (setting.lookupValue("word-bits", word_bits) ||
      setting.lookupValue("datawidth", word_bits) )
  {
    specs.word_bits = word_bits;
  }
  else
  {
    specs.word_bits = Specs::kDefaultWordBits;
  }

  // MeshX.
  std::uint32_t mesh_x;
  if (setting.lookupValue("meshX", mesh_x))
  {
    specs.meshX = mesh_x;
  }

  // MeshY.
  std::uint32_t mesh_y;
  if (setting.lookupValue("meshY", mesh_y))
  {
    specs.meshY = mesh_y;
  }

  // Network names;
  std::string operand_network_name;
  if (setting.lookupValue("network_operand", operand_network_name))
  {
    specs.operand_network_name = operand_network_name;
  }

  std::string result_network_name;
  if (setting.lookupValue("network_result", result_network_name))
  {
    specs.result_network_name = result_network_name;
  }

  // Energy (override).
  int energy_int;
  double energy;
  if (setting.lookupValue("energy", energy_int))
  {
    specs.energy_per_op = static_cast<double>(energy_int);
  }
  else if (setting.lookupValue("energy", energy))
  {
    specs.energy_per_op = energy;
  }
  else
  {
    specs.energy_per_op =
      pat::MultiplierEnergy(specs.word_bits.Get(), specs.word_bits.Get());
  }
    
  // Area (override).
  int area_int;
  double area;
  if (setting.lookupValue("area", area_int))
  {
    specs.area = static_cast<double>(area_int);
  }
  else if (setting.lookupValue("area", area))
  {
    specs.area = area;
  }
  else
  {
    specs.area =
      pat::MultiplierArea(specs.word_bits.Get(), specs.word_bits.Get());
  }

  // Validation.
  ValidateTopology(specs);

  return specs;
}

void ArithmeticUnits::ValidateTopology(ArithmeticUnits::Specs& specs)
{
  bool error = false;
  if (specs.instances.IsSpecified())
  {
    if (specs.meshX.IsSpecified())
    {
      if (specs.meshY.IsSpecified())
      {
        // All 3 are specified.
        assert(specs.meshX.Get() * specs.meshY.Get() == specs.instances.Get());
      }
      else
      {
        // Instances and MeshX are specified.
        assert(specs.instances.Get() % specs.meshX.Get() == 0);
        specs.meshY = specs.instances.Get() / specs.meshX.Get();
      }
    }
    else if (specs.meshY.IsSpecified())
    {
      // Instances and MeshY are specified.
      assert(specs.instances.Get() % specs.meshY.Get() == 0);
      specs.meshX = specs.instances.Get() / specs.meshY.Get();
    }
    else
    {
      // Only Instances is specified.
      specs.meshX = specs.instances.Get();
      specs.meshY = 1;      
    }
  }
  else if (specs.meshX.IsSpecified())
  {
    if (specs.meshY.IsSpecified())
    {
      // MeshX and MeshY are specified.
      specs.instances = specs.meshX.Get() * specs.meshY.Get();
    }
    else
    {
      // Only MeshX is specified. We can make assumptions but it's too dangerous.
      error = true;
    }
  }
  else if (specs.meshY.IsSpecified())
  {
    // Only MeshY is specified. We can make assumptions but it's too dangerous.
    error = true;
  }
  else
  {
    // Nothing is specified.
    error = true;
  }

  if (error)
  {
    std::cerr << "ERROR: " << specs.level_name
              << ": instances and/or meshX * meshY must be specified."
              << std::endl;
    exit(1);        
  }
}

// Connect networks.

void ArithmeticUnits::ConnectOperand(std::shared_ptr<Network> network)
{
  network_operand_ = network;
}

void ArithmeticUnits::ConnectResult(std::shared_ptr<Network> network)
{
  network_result_ = network;
}
  
// Accessors.

std::string ArithmeticUnits::Name() const
{
  assert(is_specced_);
  return specs_.name.Get();
}

double ArithmeticUnits::Energy(problem::Shape::DataSpaceID pv) const
{
  assert(is_evaluated_);
  assert(pv == problem::GetShape()->NumDataSpaces);
  return energy_;
}

double ArithmeticUnits::Area() const
{
  assert(is_specced_);
  return AreaPerInstance() * specs_.instances.Get();
}

double ArithmeticUnits::AreaPerInstance() const
{
  assert(is_specced_);
  return area_;
}

std::uint64_t ArithmeticUnits::Cycles() const
{
  assert(is_evaluated_);
  return cycles_;
}

std::uint64_t ArithmeticUnits::UtilizedInstances(problem::Shape::DataSpaceID pv) const
{
  (void) pv;
  assert(is_evaluated_);
  return utilized_instances_;
}

void ArithmeticUnits::Print(std::ostream& out) const
{
  std::string indent = "    ";

  // Print level name.
  out << "=== " << specs_.name << " ===" << std::endl;  
  out << std::endl;

  // Print specs.
  out << indent << "SPECS" << std::endl;
  out << indent << "-----" << std::endl;

  out << indent << "Word bits            : " << specs_.word_bits << std::endl;    
  out << indent << "Instances            : " << specs_.instances << " ("
      << specs_.meshX << "*" << specs_.meshY << ")" << std::endl;
  out << indent << "Energy-per-op        : " << specs_.energy_per_op << " pJ" << std::endl;
  out << std::endl;

  // Print stats.
  out << indent << "STATS" << std::endl;
  out << indent << "-----" << std::endl;

  out << indent << "Utilized instances   : " << UtilizedInstances() << std::endl;
  out << indent << "Cycles               : " << Cycles() << std::endl;
  out << indent << "Energy (total)       : " << Energy() << " pJ" << std::endl;
  out << indent << "Area (total)         : " << Area() << " um^2" << std::endl;
  out << std::endl;
}

} // namespace model
