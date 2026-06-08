#ifndef NCORE_REFERENCE_H
#define NCORE_REFERENCE_H

#include "Dynamic/LinkedList.h"
#include "Errors.h"

template <class TO, class FROM> class Reference : public LinkedListElement
{
    TO* iRefTo;
    FROM* iRefFrom;
protected:
    // Tell our refTo (target) object that we have a link
    virtual void targetObjectBuildLink() = 0;

    // Tell our refTo (taget) object, that the link is cut
    virtual void targetObjectDestroyLink() = 0;

    // Tell our refFrom (source) object, that the link is cut (Target destroyed)
    virtual void sourceObjectDestroyLink() = 0;
public:
    Reference() { iRefTo = nullptr; iRefFrom = nullptr; }
    ~Reference() override = default;

    // Create new link
    void link(TO* toObj, FROM* fromObj)
    {
        ASSERT(fromObj);  // fromObj MUST not be nullptr
        if (isValid())
            unlink();
        if (toObj != nullptr)
        {
            iRefTo = toObj;
            iRefFrom = fromObj;
            targetObjectBuildLink();
        }
    }

    // We don't need the reference anymore. Call comes from the refFrom object
    // Tell our refTo object, that the link is cut
    void unlink()
    {
        targetObjectDestroyLink();
        delink();
        iRefTo = nullptr;
        iRefFrom = nullptr;
    }

    // Link is invalid due to destruction of referenced target object. Call comes from the refTo object
    // Tell our refFrom object, that the link is cut
    void invalidate() // The iRefFrom MUST remain!!
    {
        sourceObjectDestroyLink();
        delink();
        iRefTo = nullptr;
    }

    [[nodiscard]] bool isValid() const  // Only check the iRefTo
    {
        return iRefTo != nullptr;
    }

    Reference* next() override { return static_cast<Reference*>(LinkedListElement::next()); }
    [[nodiscard]] Reference const* next() const override { return static_cast<Reference const*>(LinkedListElement::next()); }
    Reference* prev() override { return static_cast<Reference*>(LinkedListElement::prev()); }
    [[nodiscard]] Reference const* prev() const override { return static_cast<Reference const*>(LinkedListElement::prev()); }

    Reference* nocheck_next() override { return static_cast<Reference*>(LinkedListElement::nocheck_next()); }
    [[nodiscard]] Reference const* nocheck_next() const override { return static_cast<Reference const*>(LinkedListElement::nocheck_next()); }
    Reference* nocheck_prev() override { return static_cast<Reference*>(LinkedListElement::nocheck_prev()); }
    [[nodiscard]] Reference const* nocheck_prev() const override { return static_cast<Reference const*>(LinkedListElement::nocheck_prev()); }

    TO* operator ->() const { return iRefTo; }
    [[nodiscard]] TO* getTarget() const { return iRefTo; }
    [[nodiscard]] FROM* GetSource() const { return iRefFrom; }
};

#endif
