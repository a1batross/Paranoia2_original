
class CStartRush : public CBaseEntity
{
public:
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	CBaseEntity* GetDestinationEntity( void );
	void ReportSuccess( CBaseEntity* who );
	void Spawn( void );
	
	int ObjectCaps( void ) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
};