class OVT_BaseSaveStruct : SCR_JsonApiStruct {
	ref array<string> rdb = {};
}

[BaseContainerProps()]
class OVT_OverthrowSaveStruct : SCR_MissionStruct
{
	[Attribute()]
	protected ref OVT_EconomyStruct economy;
	
	[Attribute()]
	protected ref OVT_RealEstateStruct property;
	
	[Attribute()]
	protected ref OVT_VehiclesStruct vehicles;
	
	[Attribute()]
	protected ref OVT_OccupyingFactionStruct occupying;
	
	[Attribute()]
	protected ref OVT_ResistanceFactionStruct resistance;
	
	[Attribute()]
	protected ref OVT_JobsStruct jobs;
	
	[Attribute()]
	protected ref OVT_RadioTowerArrayStruct radioTowers;
	
	ref array<string> rdb = {};
	
	protected string occupyingFaction;
	protected ref array<int> time = {};
	protected ref array<ref OVT_TownStruct> towns = {};
	
	override bool Serialize()
	{
		rdb.Clear();
		towns.Clear();
		time.Clear();
		
		if(economy)
		{
			economy.rdb = rdb;
			if(!economy.Serialize()) return false;
		}
		if(property)
		{
			property.rdb = rdb;
			if(!property.Serialize()) return false;
		}
		if(vehicles)
		{
			vehicles.rdb = rdb;
			if(!vehicles.Serialize()) return false;
		}
		if(occupying)
		{
			occupying.rdb = rdb;
			if(!occupying.Serialize()) return false;
		}
		if(resistance)
		{
			resistance.rdb = rdb;
			if(!resistance.Serialize()) return false;
		}
		if(jobs)
		{
			jobs.rdb = rdb;
			if(!jobs.Serialize()) return false;
		}		
		
		if(radioTowers)
		{
			radioTowers.towers.Clear();
			foreach(OVT_RadioTowerData data : OVT_Global.GetOccupyingFaction().m_RadioTowers)
			{
				OVT_RadioTowerStruct struct = new OVT_RadioTowerStruct;
				struct.pos = data.location;
				struct.faction = data.faction;
				radioTowers.towers.Insert(struct);
			}
		}

		
		TimeAndWeatherManagerEntity timeMgr = GetGame().GetTimeAndWeatherManager();
		TimeContainer t = timeMgr.GetTime();
				
		int y,m,d;
		timeMgr.GetDate(y,m,d);
		
		time.Insert(y);
		time.Insert(m);
		time.Insert(d);
		time.Insert(t.m_iHours);
		time.Insert(t.m_iMinutes);
		time.Insert(t.m_iSeconds);
		
		OVT_OverthrowConfigComponent config = OVT_Global.GetConfig();
		occupyingFaction = config.m_sOccupyingFaction;
		
		foreach(OVT_TownData town : OVT_Global.GetTowns().m_Towns)
		{
			OVT_TownStruct struct = new OVT_TownStruct();
			struct.Parse(town);
			towns.Insert(struct);
		}
		
		return true;
	}
	override bool Deserialize()
	{
		if(economy)
		{
			economy.rdb = rdb;
			if(!economy.Deserialize()) return false;
		}
		if(property)
		{
			property.rdb = rdb;
			if(!property.Deserialize()) return false;
		}
		if(vehicles)
		{
			vehicles.rdb = rdb;
			if(!vehicles.Deserialize()) return false;
		}
		if(occupying)
		{
			occupying.rdb = rdb;
			if(!occupying.Deserialize()) return false;
		}
		if(resistance)
		{
			resistance.rdb = rdb;
			if(!resistance.Deserialize()) return false;
		}
		if(jobs)
		{
			jobs.rdb = rdb;
			if(!jobs.Deserialize()) return false;
		}	
		
		if(radioTowers)
		{
			foreach(OVT_RadioTowerStruct struct : radioTowers.towers)
			{
				OVT_RadioTowerData data = OVT_Global.GetOccupyingFaction().GetNearestRadioTower(struct.pos);
				if(data){
					data.faction = struct.faction;
				}
			}
		}
		
		//A workaround because this will be fired with a blank struct on dedi servers that have no existing save file
		if(occupyingFaction == "") return true;
					
		OVT_OverthrowConfigComponent config = OVT_Global.GetConfig();
		config.SetOccupyingFaction(occupyingFaction);
		
		if(time)
		{
			TimeAndWeatherManagerEntity timeMgr = GetGame().GetTimeAndWeatherManager();
			timeMgr.SetDate(time[0], time[1], time[2], true);
			TimeContainer t = timeMgr.GetTime();
			t.m_iHours = time[3];
			t.m_iMinutes = time[4];
			t.m_iSeconds = time[5];
			timeMgr.SetTime(t);
		}
		
		foreach(OVT_TownData town : OVT_Global.GetTowns().m_Towns)
		{			
			foreach(OVT_TownStruct struct : towns)
			{
				float dist = vector.Distance(struct.pos, town.location);
				if(dist < 50)
				{
					town.population = struct.pop;
					town.support = struct.support;
					town.gunDealerPosition = struct.gunDealerPos;
					foreach(int index, int i : struct.supportMods)
					{
						OVT_TownModifierData data = new OVT_TownModifierData;
						data.id = i;
						data.timer = struct.supportModTime[index];
						town.supportModifiers.Insert(data);
					}
					foreach(int index, int i : struct.stabilityMods)
					{
						OVT_TownModifierData data = new OVT_TownModifierData;
						data.id = i;
						data.timer = struct.stabilityModTime[index];
						town.stabilityModifiers.Insert(data);
					}					
					
					Faction fac = GetGame().GetFactionManager().GetFactionByKey(struct.faction);
					if(fac)
					{
						town.faction = GetGame().GetFactionManager().GetFactionIndex(fac);
					}
					break;
				}
			}
		}
		
		OVT_OverthrowGameMode mode = OVT_OverthrowGameMode.Cast(GetGame().GetGameMode());
		mode.DoStartGame();
		
		return true;
	}
	
	void OVT_OverthrowSaveStruct()
	{
		RegV("rdb");
		RegV("economy");
		RegV("property");
		RegV("vehicles");
		RegV("occupying");
		RegV("resistance");
		RegV("occupyingFaction");
		RegV("jobs");
		RegV("towns");
		RegV("time");
		RegV("radioTowers");
	}
}

class OVT_TownStruct : SCR_JsonApiStruct
{
	vector pos;
	vector gunDealerPos;
	int pop;
	int support;
	string faction;
	ref array<ref int> supportMods = {};
	ref array<ref int> supportModTime = {};
	ref array<ref int> stabilityMods = {};	
	ref array<ref int> stabilityModTime = {};
	
	void Parse(OVT_TownData town)
	{
		FactionManager factions = GetGame().GetFactionManager();
		
		pos = town.location;
		gunDealerPos = town.gunDealerPosition;
		pop = town.population;
		support = town.support;
		faction = factions.GetFactionByIndex(town.faction).GetFactionKey();
		
		supportMods.Clear();
		supportModTime.Clear();
		stabilityMods.Clear();
		stabilityModTime.Clear();
		
		foreach(OVT_TownModifierData data : town.supportModifiers)
		{
			if(!data) continue;
			supportMods.Insert(data.id);
			supportModTime.Insert(data.timer);
		}
		
		foreach(OVT_TownModifierData data : town.stabilityModifiers)
		{
			if(!data) continue;
			stabilityMods.Insert(data.id);
			stabilityModTime.Insert(data.timer);
		}
	}
	
	void OVT_TownStruct()
	{
		RegV("pos");
		RegV("gunDealerPos");
		RegV("pop");
		RegV("support");
		RegV("faction");
		RegV("supportMods");
		RegV("supportModTime");
		RegV("stabilityMods");
		RegV("stabilityModTime");
	}
}

[BaseContainerProps()]
class OVT_RadioTowerArrayStruct : SCR_JsonApiStruct
{
	ref array<ref OVT_RadioTowerStruct> towers = {};
	void OVT_RadioTowerArrayStruct()
	{
		RegV("towers");
	}
}

class OVT_RadioTowerStruct : SCR_JsonApiStruct
{
	vector pos;
	int faction;
	ref array<int> garrison = {};
	void OVT_RadioTowerStruct()
	{
		RegV("pos");
		RegV("faction");
		RegV("garrison");
	}
}

class OVT_InventoryStruct : SCR_JsonApiStruct
{
	int id;
	int qty;
	void OVT_InventoryStruct()
	{
		RegV("id");
		RegV("qty");
	}
}

class OVT_IDMapStruct : SCR_JsonApiStruct
{
	int id;
	ref set<int> ids;
	void OVT_IDMapStruct()
	{
		RegV("id");
		RegV("ids");
	}
}

class OVT_PlayerIDIntMapStruct : SCR_JsonApiStruct
{
	string id;
	ref array<ref OVT_InventoryStruct> ids = {};
	void OVT_IDMapStruct()
	{
		RegV("id");
		RegV("ids");
	}
}