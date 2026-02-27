template <class T> class Iterator
{
public:
   Iterator(tInventory* i) : inventory(i)
   {
      inventory->AddRef();
      it = new tEntityTable::RawIterator(inventory->GetCurrentSection());
   }

   Iterator() : inventory(tPlatform::GetPlatform()->GetActiveContext()->GetInventory())
   {
      inventory->AddRef();
      it = new tEntityTable::RawIterator(inventory->GetCurrentSection());
   }

   ~Iterator()
   {
      inventory->Release();
      delete it;
   }

   inline T* First()
   {
       radLoadInventory::SafeCast<T> cast;
       tEntity* obj = static_cast<tEntity*>(it->First());
       return cast.safe_cast(obj) ? (T*)obj : (T*)Next();
   }

   inline T* Last()
   {
      radLoadInventory::SafeCast<T> cast;
      tEntity* obj = static_cast<tEntity*>(it->Last());
      return cast.safe_cast(obj) ? (T*)obj : (T*)Prev();
   }

   inline T* Next()
   {
      radLoadInventory::SafeCast<T> cast;
      tEntity* obj = static_cast<tEntity*>(it->Next());
      while(obj && !cast.safe_cast(obj))
      {
         obj = (T*)it->Next();
      }
      return (T*)obj;
   }

   inline T* Prev()
   {
      radLoadInventory::SafeCast<T> cast;
      tEntity* obj = static_cast<tEntity*>(it->Prev());
      while(obj && !cast.safe_cast(obj))
      {
         obj = (T*)it->Prev();
      }
      return (T*)obj;
   }

private:
   tInventory* inventory;
   tEntityTable::RawIterator* it;
};
