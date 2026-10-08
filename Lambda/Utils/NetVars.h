#pragma once

#include <string>

struct inputdata_t;
typedef enum _fieldtypes {
	FIELD_VOID = 0,			
	FIELD_FLOAT,			
	FIELD_STRING,			
	FIELD_VECTOR,			
	FIELD_QUATERNION,		
	FIELD_INTEGER,			
	FIELD_BOOLEAN,			
	FIELD_SHORT,			
	FIELD_CHARACTER,		
	FIELD_COLOR32,			
	FIELD_EMBEDDED,			
	FIELD_CUSTOM,			

	FIELD_CLASSPTR,			
	FIELD_EHANDLE,			
	FIELD_EDICT,			

	FIELD_POSITION_VECTOR,	
	FIELD_TIME,				
	FIELD_TICK,				
	FIELD_MODELNAME,		
	FIELD_SOUNDNAME,		

	FIELD_INPUT,			
	FIELD_FUNCTION,			

	FIELD_VMATRIX,			

							
							FIELD_VMATRIX_WORLDSPACE,
							FIELD_MATRIX3X4_WORLDSPACE,	

							FIELD_INTERVAL,			
							FIELD_MODELINDEX,		
							FIELD_MATERIALINDEX,	

							FIELD_VECTOR2D,			

							FIELD_TYPECOUNT,		
} fieldtype_t;

class ISaveRestoreOps;
class C_BaseEntity;



typedef void (C_BaseEntity::* inputfunc_t)(inputdata_t& data);

struct datamap_t;
class RecvTable;
class typedescription_t;

enum {
	TD_OFFSET_NORMAL = 0,
	TD_OFFSET_PACKED = 1,

	
	TD_OFFSET_COUNT,
};

class typedescription_t {
public:
	int fieldType; 
	char* fieldName; 
	int fieldOffset[TD_OFFSET_COUNT]; 
	short fieldSize_UNKNWN; 
	short flags_UNKWN; 
	char pad_0014[12]; 
	datamap_t* td; 
	char pad_0024[24]; 
}; 


   
   
   
   
struct datamap_t {
	typedescription_t* m_data_desc;
	int                    m_data_num_fields;
	char const* m_data_class_name;
	datamap_t* m_base_map;

	bool                m_chains_validated;
	
	bool                m_packed_offsets_computed;
	int                    m_packed_size;
};


class RecvProp;
struct LuaProp_t {
	RecvProp* prop;
	int offset = 0;
};

namespace NetVars {
	LuaProp_t FindProp(RecvTable* table, std::string netvar);
	int	GetNetVar(const char* table, const char* netvar);
	int GetNetVar(RecvTable* table, const char* netvar);
	int FindInDataMap(datamap_t* map, const char* netvar);
};