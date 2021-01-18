#pragma once

class CAnimBlendAssociation;

class CAnimBlendAssocGroup
{
public:
	CAnimBlendAssociation *assocList;
	int32 numAssociations;

	CAnimBlendAssocGroup(void);
	~CAnimBlendAssocGroup(void);
	void DestroyAssociations(void);
	CAnimBlendAssociation *GetAnimation(uint32 id);
	CAnimBlendAssociation *GetAnimation(const char *name);
	CAnimBlendAssociation *CopyAnimation(uint32 id);
	CAnimBlendAssociation *CopyAnimation(const char *name);
#ifdef ADAPT_PED_HIERARCHY
	CAnimBlendAssociation *CopyAnimation(uint32 id, RpClump *clump);
	CAnimBlendAssociation *CopyAnimation(const char *name, RpClump *clump);
#endif
	void CreateAssociations(const char *name);
	void CreateAssociations(const char *blockName, RpClump *clump, const char **animNames, int numAssocs);
};
