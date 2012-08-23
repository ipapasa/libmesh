// $Id: factory.h,v 1.2 2004-01-03 15:37:41 benkirk Exp $

// The libMesh Finite Element Library.
// Copyright (C) 2002-2004  Benjamin S. Kirk, John W. Peterson
  
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
  
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
  
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA



#ifndef __factory_h__
#define __factory_h__


// System & C++ includes
#include <string>
#include <map>

// Local includes
#include "libmesh_common.h"
#include "auto_ptr.h"


/**
 * A base class.
 */
class FactoryBase
{
public:

protected:
  /**
   * Constructor.
   */
  FactoryBase () {}

  /**
   * Destructor.
   */
  virtual ~FactoryBase () {}  
};



/**
 * Factory class defintion.  
 */
template <class Base>
class Factory : public FactoryBase
{  
protected:

  /**
   * Constructor. Takes the name to be mapped.
   */
  Factory (const std::string& name);

public:

  /**
   * Destructor. (Empty.)
   */
  virtual ~Factory () {}

  /**
   * Builds an object of type Base identified by name.
   */
  static AutoPtr<Base> build (const std::string& name);

  /**
   * Create a Base class.  Force this to be implemented
   * later.
   */
  virtual AutoPtr<Base> create () = 0;

private:

  /**
   * Map from a name to a Factory<Base>* pointer.
   */
  static std::map<std::string, FactoryBase*> factory_map;
};



/**
 * Factory implementation class.
 */
template <class Derived, class Base>
class FactoryImp: public Factory<Base>
{
public:

  /**
   * Constructor.  Takes a name as input.
   */
  FactoryImp (const std::string& name) : Factory<Base>(name) { }

  /**
   * Destructor.  Empty.
   */
  ~FactoryImp () {}

private:

  /**
   * @returns a new object of type Derived. 
   */
  AutoPtr<Base> create ();

};



// -----------------------------------------------------
// Factory members
template <class Base>
inline
Factory<Base>::Factory (const std::string& name)
{
  // Make sure we haven't already added this name
  // to the map
  assert (!factory_map.count(name));

  factory_map[name] = this;
}



template <class Base>
inline
AutoPtr<Base> Factory<Base>::build (const std::string& name)
{
  // name not found in the map
  if (!factory_map.count(name))
    {
      std::cerr << "Tried to build an unknown type: " << name << std::endl;

      std::cerr << "valid options are:" << std::endl;
      
      for (typename std::map<std::string,FactoryBase*>::const_iterator
	     it = factory_map.begin(); it != factory_map.end(); ++it)
        std::cerr << "  " << it->first << std::endl;

      // Do this the stoopid way for IBM xlC
      AutoPtr<Base> ret_val (NULL);
      
      return ret_val;
    }
  
  // Do this the stoopid way for IBM xlC
  Factory<Base> *f = dynamic_cast<Factory<Base>*> (factory_map[name]);
  
  AutoPtr<Base> ret_val (f->create());
  
  return ret_val;
}



template <class Derived, class Base>
inline
AutoPtr<Base> FactoryImp<Derived,Base>::create ()
{  
  // Do this the stoopid way for IBM xlC
  AutoPtr<Base> ret_val (new Derived);

  return ret_val;
}


#endif