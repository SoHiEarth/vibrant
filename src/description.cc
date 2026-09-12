#include "description.h"

const std::map<std::string, std::string> kDescriptionMap = {
  {"transform.position", "The position of the object."},
  {"transform.scale", "The scale of the object."},
  {"transform.rotation", "The rotation of the object"},
  {"light.type", "0: Directional Light, 1: Point Light. Other values are ignored."},
  {"light.intensity", "The intensity of the light."},
  {"light.color", "The color of the light."},
  {"light.radial_falloff", "The range of the light"},
  {"light.volumetric_intensity", "The amount of which the light effects the volume around it."},
  {"texture.color", "The color texture of the object. (Optional. Removing it will set the color to black.)"},
  {"texture.normal", "The normal texture of the object. (Optional. Removing it will shade it flat.)"}
};
