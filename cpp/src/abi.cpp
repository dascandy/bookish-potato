#include <cstdint>
#include <cstddef>

namespace std
{
  class type_info
  {
  public:
    virtual ~type_info();
    const char* name() const { return __name[0] == '*' ? __name + 1 : __name; }

    bool before(const type_info& __arg) const 
    { return __name < __arg.__name; }

    bool operator==(const type_info& __arg) const 
    { return __name == __arg.__name; }

    size_t hash_code() const noexcept
    {
      return reinterpret_cast<size_t>(__name);
    }

    virtual bool __is_pointer_p() const;
    virtual bool __is_function_p() const;
    /*
    virtual bool __do_catch(const type_info *__thr_type, void **__thr_obj, unsigned __outer) const { 
      (void)__thr_type;
      (void)__thr_obj;
      return true; 
    }
    virtual bool __do_upcast(const __cxxabiv1::__class_type_info *__target, void **__obj_ptr) const { 
      (void)__target;
      (void)__obj_ptr;
      return true; 
    }
*/
  protected:
    const char *__name;
    explicit type_info(const char *__n): __name(__n) { }

  private:
    type_info& operator=(const type_info&);
    type_info(const type_info&);
  };
  type_info::~type_info() {}
  bool type_info::__is_pointer_p() const { return true; }
}

namespace __cxxabiv1
{
  class __function_type_info : public std::type_info
  {
  public:
    explicit __function_type_info(const char* __n) : std::type_info(__n) { }
    virtual ~__function_type_info();
  protected:
    virtual bool __is_function_p() const;
  };
  __function_type_info::~__function_type_info() {}
  bool __function_type_info::__is_function_p() const { return true; }
}

