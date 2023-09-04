#ifdef _MSC_VER

#pragma once

/*
    ��������������� ������ � ������� ��� ������ � COM ���������
    ���������� ���������� ��� �ComPtr � CComQIPtr - ComPtr
    ���������� ��������������� ������� ��� �������� COM ��������
*/

#include <atlbase.h>
#include <atlcom.h>
#include <atlcomcli.h>

#include <combaseapi.h> // ��� DECLARE_INTERFACE_IID_
#include <stdio.h>      // ��� __VA_ARGS__

/*
�������������:
    ANNOUNCE_INTERFACE();

    ICustomInterfacePtr pInterface = nullptr;
*/
// ���������� ���������� ��� ��� ��������
// �������� typedef �� ComPtr �� ����������
// TODO check _COM_SMARTPTR_TYPEDEF
#define ANNOUNCE_INTERFACE(iface)   \
interface iface;                    \
typedef ComPtr<iface> iface##Ptr;   \

/*
�������������:
    DECLARE_COM_INTERFACE_(ICustomInterface, "515CB11E-2B96-4685-9555-69B28A89B830")
    {
        virtual void MyFunc() = 0;
    };

    ICustomInterfacePtr pInterface = nullptr;
*/
// ���������� COM ���������� � ��� ��������������
// �������� typedef �� ComPtr �� ����������
#define DECLARE_COM_INTERFACE(iface, uuid)      \
ANNOUNCE_INTERFACE(iface)                       \
DECLARE_INTERFACE_IID_(iface, IUnknown, uuid)

/*
�������������:
    DECLARE_COM_INTERFACE_(ICustomInterface, IUnknown, "515CB11E-2B96-4685-9555-69B28A89B830")
    {
        virtual void MyFunc() = 0;
    };

    ICustomInterfacePtr pInterface = nullptr;
*/
// ���������� COM ����������, ������������ �� baseiface � ��� ��������������
// �������� typedef �� ComPtr �� ����������
#define DECLARE_COM_INTERFACE_(iface, baseiface, uuid)  \
ANNOUNCE_INTERFACE(iface)                               \
DECLARE_INTERFACE_IID_(iface, baseiface, uuid)

/*
�������������:
    DECLARE_COM_INTERNAL_CLASS(CustomClass)
        , public ICustomInterface
    {
    public:
        CustomClass() {}
        CustomClass(int a) {}

        BEGIN_COM_MAP(CustomClass)
            COM_INTERFACE_ENTRY(ICustomInterface)
        END_COM_MAP()

    // ICustomInterface
    public:
        void MyFunc() override {}
    };

    CustomClass::Ptr pClass = CustomClass::create();
    CustomClass::Ptr pClass = CustomClass::make(10);
    ICustomInterfacePtr pInterface = CustomClass::make(10);
*/
// ���������� ����������� COM ������, ��������� ����������� ��� ������ � COM ������
#define DECLARE_COM_INTERNAL_CLASS(classname)       \
class ATL_NO_VTABLE classname                       \
    : public CComObjectRootEx<CComMultiThreadModel> \
    , public COMInternalClass<classname>
#define DECLARE_COM_INTERNAL_CLASS_TEMPLATE(classname, ...) \
class ATL_NO_VTABLE classname                               \
    : public CComObjectRootEx<CComMultiThreadModel>         \
    , public COMInternalClass<classname<__VA_ARGS__>>

/*
�������������:
    DECLARE_COM_INTERNAL_CLASS_(CustomClassWithBase, BaseClass)
        , public ICustomInterface
    {
    public:
        CustomClassWithBase() {}
        CustomClassWithBase(int a) : BaseClass(a) {}

        BEGIN_COM_MAP(CustomClassWithBase)
            COM_INTERFACE_ENTRY(ICustomInterface)
            COM_INTERFACE_ENTRY_CHAIN(BaseClass)
        END_COM_MAP()

    // ICustomInterface
    public:
        void MyFunc() override {}
    };

    CustomClassWithBase::Ptr pClass = CustomClassWithBase::create();
    CustomClassWithBase::Ptr pClass = CustomClassWithBase::make(10);
    ICustomInterfacePtr pInterface = CustomClassWithBase::make(10);
*/
// ���������� ����������� COM ������, ������������ �� ������� COM ������, �� DECLARE_COM_BASE_CLASS
#define DECLARE_COM_INTERNAL_CLASS_(classname, baseCOMClass)    \
class ATL_NO_VTABLE classname                                   \
    : public COMInternalClass<classname>                        \
    , public baseCOMClass
#define DECLARE_COM_INTERNAL_CLASS_TEMPLATE_(classname, baseCOMClass, ...)  \
class ATL_NO_VTABLE classname                                               \
    : public COMInternalClass<classname<__VA_ARGS__>>                       \
    , public baseCOMClass

/*
�������������:
    DECLARE_COM_BASE_CLASS(COMBase)
        , public ICustomInterface
    {
    public:
        COMBase() = default;

        BEGIN_COM_MAP(COMBase)
            COM_INTERFACE_ENTRY(ICustomInterface)
        END_COM_MAP()
    };
};
*/
// ��������� ������� ����� � ������� ����� ���� COM_MAP � ���������� �������
// �� ��������� ��� ������� ������, ��� ����� ����������� ������� ����������� COM ��������
#define DECLARE_COM_BASE_CLASS(classname)               \
class ATL_NO_VTABLE classname                           \
    : public CComObjectRootEx<CComMultiThreadModel>

////////////////////////////////////////////////////////////////////////////////
// ����������� ����� CComQIPtr, ����� �������������� �� ����� ������������ CComPtr ��������
// ���� �� CComPtr, CComQIPtr, CComPtrBase
// TODO see _com_ptr_t
template <class T>
class ComPtr :
    public CComPtr<T>
{
public:
    ComPtr() throw()
    {
    }
    ComPtr(decltype(__nullptr)) throw()
    {
    }
    ComPtr(_Inout_opt_ T* lp) throw() :
        CComPtr<T>(lp)
    {
    }
    ComPtr(_Inout_opt_ IUnknown* lp) throw()
    {
        if (lp != NULL)
        {
            if (FAILED(lp->QueryInterface(__uuidof(T), (void **)&this->p)))
                this->p = NULL;
        }
    }
    ComPtr(_Inout_ const ComPtr<T>& lp) throw() :
        CComPtr<T>(lp.p)
    {
    }
    ComPtr(_Inout_ const CComPtrBase<T>& lp) throw() :
        CComPtr<T>(lp.p)
    {
    }
    template <typename U>
    ComPtr(_Inout_ const CComPtrBase<U>& lp)
    {
        if (lp != NULL)
        {
            if (FAILED(lp->QueryInterface(__uuidof(T), (void **)&this->p)))
                this->p = NULL;
        }
    }
    // ����� ��� ������������ CComPtr()
    template <typename U>
    operator CComPtr<U>() const noexcept
    {
        CComPtr<U> temp;
        this->QueryInterface(&temp);
        return temp;
    }
    // �������� & �������� ������ �������� ���� ������� �����, � ��������� ������
    T** address() noexcept
    {
        return &this->p;
    }
    T* get() noexcept
    {
        return this->p;
    }
};

////////////////////////////////////////////////////////////////////////////////
// ����������� ������ CComObjectNoLock
// Base is the user's class that derives from CComObjectRoot and whatever
// interfaces the user wants to support on the object
template <class Base>
class ComObjectNoLock :
    public Base
{
public:
    typedef Base _BaseClass;

    template <typename ...Args>
    ComObjectNoLock(_In_ Args&&... args)
        : Base(std::forward<Args>(args)...)
    {
    }

    // Set refcount to -(LONG_MAX/2) to protect destruction and
    // also catch mismatched Release in debug builds

    virtual ~ComObjectNoLock()
    {
        this->m_dwRef = -(LONG_MAX / 2);
        this->FinalRelease();
#if defined(_ATL_DEBUG_INTERFACES) && !defined(_ATL_STATIC_LIB_IMPL)
        _AtlDebugInterfacesModule.DeleteNonAddRefThunk(_GetRawUnknown());
#endif
    }

    //If InternalAddRef or InternalRelease is undefined then your class
    //doesn't derive from CComObjectRoot
    STDMETHOD_(ULONG, AddRef)() throw()
    {
        return this->InternalAddRef();
    }
    STDMETHOD_(ULONG, Release)() throw()
    {
        ULONG l = this->InternalRelease();
        if (l == 0)
            delete this;
        return l;
    }
    //if _InternalQueryInterface is undefined then you forgot BEGIN_COM_MAP
    STDMETHOD(QueryInterface)(
        REFIID iid,
        _COM_Outptr_ void** ppvObject) throw()
    {
        return this->_InternalQueryInterface(iid, ppvObject);
    }
};

///////////////////////////////////////////////////////////////////////////////
/// Com private base class
template <typename Base>
class COMInternalClass
{
public:
    /// type of an object creatable via create call
    /**
     * You can change this type to, say, CComPolyObject<Base> by defining
     * FinalType type inside your class.
     */
    typedef ComObjectNoLock<Base> FinalType;

    /// type of an object creatable via aggregate call
    /**
     * You can change this type to, say, CComPolyObject<Base> by defining 
     * FinalType type inside your class.
     * @see FinalType
     */
    typedef CComAggObject<Base> FinalAggType;

    /// smart pointer type
    typedef ComPtr<Base> Ptr;

    /// makes an object Ptr with ctor parameters
    template <typename ...Args>
    static Base* makeRaw(Args&&... args)
    {
        typedef typename Base::FinalType FinalType;
        FinalType* p = new FinalType(std::forward<Args>(args)...);
        p->SetVoid(0);
        p->InternalFinalConstructAddRef();
        HRESULT hRes = p->_AtlInitialConstruct();
        if (SUCCEEDED(hRes))
            hRes = p->FinalConstruct();
        p->InternalFinalConstructRelease();
        if (hRes != S_OK)
        {
            delete p;
            return nullptr;
        }
        return static_cast<Base*>(p);
    }

    /// makes an object Ptr with ctor parameters
    template <typename ...Args>
    static ComPtr<Base> make(Args&&... args)
    {
        return ComPtr<Base>(makeRaw(std::forward<Args>(args)...));
    }

    /// creates an object pointer
    static Base* createRaw()
    {
        return makeRaw();
    }

    /// creates an object Ptr
    static ComPtr<Base> create()
    {
        return make();
    }

    /*
    AggregateExample:
        DECLARE_COM_INTERNAL_CLASS(AggregateClass) , public IInterface
        {
        public:
            BEGIN_COM_MAP(AggregateClass)
                COM_INTERFACE_ENTRY(IInterface)
            END_COM_MAP()

            AggregateClass() = default;
        };

        DECLARE_COM_INTERNAL_CLASS(InternalClass) , public IInterfaceSecond
        {
        public:
            DECLARE_PROTECT_FINAL_CONSTRUCT();  // !!!

            BEGIN_COM_MAP(InternalClass)
                COM_INTERFACE_ENTRY(IInterfaceSecond)
                COM_INTERFACE_ENTRY_AGGREGATE_BLIND(m_unknown)
            END_COM_MAP()

            HRESULT FinalConstruct()
            {
                m_aggregated = AggregateClass::aggregate(GetUnknown(), m_unknown);
                return S_OK;
            }

            void FinalRelease() { m_unknown = nullptr; }

        private:
            AggregateClass* m_aggregated;
            IUnknownPtr m_unknown;
        };
    */
    /// creates an object of FinalAggType as a part of pOuter aggregate
    /// see CComAggObject::CreateInstance
    static Base* aggregate(LPUNKNOWN pOuter, ComPtr<IUnknown>& ptr)
    {
        typedef typename Base::FinalAggType FinalAggType;
        FinalAggType* p = new FinalAggType(pOuter);
        p->SetVoid(pOuter);
        p->InternalFinalConstructAddRef();
        HRESULT hRes = p->_AtlInitialConstruct();
        if (SUCCEEDED(hRes))
            hRes = p->FinalConstruct();
        p->InternalFinalConstructRelease();
        if (hRes != S_OK)
        {
            delete p;
            return nullptr;
        }
        ptr = p;
        return static_cast<Base*>(&p->m_contained);
    }
};

/*
    ��������� �� ���������� "����������" COM ������, � ������������ � ��� �����

*/

/*
#define DECLARE_COM_CLASS_UUID(classname, uuid)         \
    struct DECLSPEC_UUID(iid) classname##_COM_CLASS;

#define DECLARE_COM_CLASS(classname, uuid)              \
    struct DECLSPEC_UUID(uuid) classname##_COM_CLASS;   \
    class ATL_NO_VTABLE classname :                     \
        public CComObjectRootEx<CComMultiThreadModel>,  \
        public COMClassBase<classname,                  \
                           &_ATL_IIDOF(classname##_COM_CLASS)>,


#define GET_CLASS_UUID(classname) _ATL_IIDOF(classname##_COM_CLASS)

template <class I>
ComPtr<I> CreateInstance(REFCLSID clsid)
{
    ComPtr<I> res;
    if (FAILED(res.CoCreateInstance(clsid)))
        return nullptr;
    else
        return res;
}

template <typename Base, const CLSID* pclsid>
class ATL_NO_VTABLE COMClassBase :
    public CComCoClass<Base, pclsid>,
    public COMInternalClass<Base>
{
protected:
    /// definition of base SCOM class for use in final implementation
    typedef COMClassBase<Base, pclsid> COM_Base;

public:
    // ATL aggregatable type
    //DECLARE_AGGREGATABLE(Base)
    //typedef ATL::CComCreator2<ATL::CComCreator<typename COM_InternalClass<Base>::FinalType>, ATL::CComCreator<ATL::CComAggObject<Base> > > _CreatorClass;
    typedef ATL::CComCreator2< ATL::CComCreator< ATL::CComObjectNoLock< Base > >, ATL::CComCreator< ATL::CComAggObject< Base > > > _CreatorClass;

    // don't let class factory refcount influence lock count
    DECLARE_CLASSFACTORY()
};

template <class T>
class COMRegisterClass
{
public:
    COMRegisterClass(REFCLSID clsid)
    {
        /*
            Register a COM class in-memory.
            Any requests by COM to create object clsid will use the
            class factory given by factory.
            This means that a COM object does not have to be registered
            in the registry in order to use it. Nor does it have
            to be included in an application manifest.

            Public domain: No attribution required.
        *//*

        IClassFactory* pCF;
        T::_CreatorClass::CreateInstance(NULL,
                                         IID_IClassFactory,
                                         (void**)&pCF);

        DWORD dwCookie; //returned cookie can be later used to delete the registration
        HRESULT hr = ::CoRegisterClassObject(clsid,
                                             static_cast<IUnknown*>(pCF),
                                             CLSCTX_LOCAL_SERVER,
                                             REGCLS_SINGLEUSE,
                                             &dwCookie);

    }
};

// ����������� COM ������
#define REGISTER_COM_CLASS(classname)   \
    static RegisterClassMy<classname> s_classRegistrator##classname(GET_CLASS_UUID(classname));
*/

#endif // _MSC_VER
