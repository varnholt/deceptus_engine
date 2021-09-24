#include "tmxobjectgroup.h"

#include "tmxobject.h"

#include <iostream>


TmxObjectGroup::TmxObjectGroup()
{
   _type = TmxElement::TypeObjectGroup;
}


TmxObjectGroup::~TmxObjectGroup()
{
   for (auto& [k, v] : _objects)
   {
      delete v;
   }

   _objects.clear();
}


void TmxObjectGroup::deserialize(tinyxml2::XMLElement* xml_element)
{
   TmxElement::deserialize(xml_element);

   tinyxml2::XMLNode* node = xml_element->FirstChild();
   while(node != nullptr)
   {
      tinyxml2::XMLElement* sub_element = node->ToElement();
      if (sub_element != nullptr)
      {
         TmxElement* element = nullptr;
         TmxObject* object = nullptr;

         if (sub_element->Name() == std::string("object"))
         {
            object = new TmxObject();
            element = object;
         }

         if (element != nullptr)
         {
            element->deserialize(sub_element);
         }
         else
         {
            std::cerr << sub_element->Name() << " is not supported for TmxObjectGroup" << std::endl;
         }

         if (object != nullptr)
         {
            _objects[object->_id] = object;
         }
      }

      node = node->NextSibling();
   }
}

/*
 <objectgroup name="objects">
  <object id="8" name="walk32" x="120.5" y="73.5" width="143" height="2.5"/>
  <object id="9" name="walk31" x="169" y="121" width="141.5" height="2.5"/>
  <object id="10" name="walk30" x="313" y="144.5" width="23.5" height="4"/>
  <object id="11" name="walk29" x="362.5" y="145.5" width="45.5" height="4"/>
  <object id="12" name="walk28" x="338" y="193.5" width="67.5" height="5"/>
  <object id="13" name="walk27" x="434.5" y="240.5" width="46" height="4.5"/>
  <object id="14" name="walk26" x="264.5" y="264.5" width="47.5" height="4"/>
  <object id="15" name="walk25" x="336.5" y="289" width="47" height="4"/>
  <object id="16" name="walk24" x="265" y="312.5" width="46" height="4.5"/>
  <object id="17" name="walk23" x="337.5" y="337" width="45.5" height="3.5"/>
  <object id="18" name="walk22" x="265.5" y="361" width="45.5" height="3.5"/>
  <object id="19" name="walk21" x="337" y="385" width="46.5" height="3.5"/>
  <object id="20" name="walk20" x="264.5" y="409" width="46.5" height="2.5"/>
  <object id="21" name="walk19" x="336" y="432" width="48" height="4.5"/>
  <object id="22" name="walk18" x="265" y="457" width="47" height="4"/>
  <object id="23" name="walk17" x="120" y="504.5" width="287" height="3.5"/>
  <object id="25" name="walk16" x="335.5" y="479.5" width="48.5" height="3.5"/>
  <object id="26" name="walk15" x="481.5" y="168" width="23" height="5"/>
  <object id="27" name="walk14" x="506" y="119.5" width="21" height="6"/>
  <object id="28" name="walk13" x="553" y="120" width="22" height="4.5"/>
  <object id="29" name="walk12" x="577.5" y="168" width="20.5" height="3.5"/>
  <object id="30" name="walk11" x="600" y="239.5" width="46.5" height="5"/>
  <object id="31" name="walk10" x="673.5" y="192.5" width="69.5" height="4.5"/>
  <object id="32" name="walk9" x="673.5" y="144" width="21.5" height="5.5"/>
  <object id="33" name="walk8" x="720.5" y="240" width="23.5" height="3"/>
  <object id="34" name="walk7" x="745" y="265.5" width="22.5" height="3.5"/>
  <object id="35" name="walk6" x="768" y="288.5" width="23" height="3.5"/>
  <object id="36" name="walk5" x="793.5" y="312.5" width="22" height="4"/>
  <object id="37" name="walk4" x="817" y="337" width="22.5" height="4.5"/>
  <object id="38" name="walk3" x="840.5" y="360" width="23" height="4.5"/>
  <object id="39" name="walk2" x="864" y="384" width="24" height="4"/>
  <object id="40" name="walk1" x="889.5" y="409.5" width="22.5" height="3"/>
  <object id="41" name="walkpoly2" x="463.5" y="311">
   <polyline points="0,0 20.5,1 19,-68.5 43.5,-70 44.5,-141.5 64,-141 67.5,-214 87.5,-212 88,-141.5 111.5,-141 109.5,-69 135.5,-69 134.5,0 158,1.5 158,21.5 -3,20.5 0,-1"/>
  </object>
  <object id="42" name="walkpoly1" x="384.5" y="265">
   <polyline points="0,0 22,-0.5 21.5,96 93.5,96.5 94.5,169.5 218,168.5 218.5,97.5 291.5,97.5 290,-1.5 309.5,-1.5 310,118.5 236.5,117.5 237.5,189 74.5,189.5 74.5,117 1,118 0,3"/>
  </object>
  <object id="43" name="kill11" x="518" y="145" width="9" height="22"/>
  <object id="44" name="kill10" x="554" y="145" width="7.5" height="21.5"/>
  <object id="45" name="kill9" x="494.5" y="195" width="9.5" height="33"/>
  <object id="46" name="kill8" x="481.5" y="230.5" width="23.5" height="7"/>
  <object id="47" name="kill7" x="577.5" y="193.5" width="7.5" height="32"/>
  <object id="48" name="kill6" x="577" y="228.5" width="21.5" height="9"/>
  <object id="49" name="kill5" x="469.5" y="264.5" width="10" height="37.5"/>
  <object id="50" name="kill4" x="458.5" y="301" width="19.5" height="8"/>
  <object id="51" name="kill3" x="504.5" y="336.5" width="69" height="9.5"/>
  <object id="52" name="kill2" x="600" y="265" width="9" height="35"/>
  <object id="53" name="kill1" x="601" y="304.5" width="21.5" height="4.5"/>
 </objectgroup>

*/
