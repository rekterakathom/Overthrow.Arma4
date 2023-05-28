class OVT_OverthrowGameModeClass: SCR_BaseGameModeClass
{
};

class OVT_OverthrowGameMode : SCR_BaseGameMode
{
	[Attribute()]
	ref OVT_UIContext m_StartGameUIContext;
	
	[Attribute()]
	ResourceName m_PlayerCommsPrefab;
	
	[Attribute(uiwidget: UIWidgets.ResourceNamePicker, desc: "Start Camera Prefab", params: "et")]
	ResourceName m_StartCameraPrefab;
	
	protected OVT_OverthrowConfigComponent m_Config;
	protected OVT_TownManagerComponent m_TownManager;
	protected OVT_OccupyingFactionManager m_OccupyingFactionManager;
	protected OVT_ResistanceFactionManager m_ResistanceFactionManager;
	protected OVT_RealEstateManagerComponent m_RealEstate;
	protected OVT_VehicleManagerComponent m_VehicleManager;
	protected OVT_EconomyManagerComponent m_EconomyManager;
	protected OVT_PlayerManagerComponent m_PlayerManager;
	protected OVT_JobManagerComponent m_JobManager;
	protected OVT_PersistenceManagerComponent m_Persistence;
	
	OVT_PlayerCommsEntity m_Server;
	
	ref set<string> m_aInitializedPlayers;
	ref set<string> m_aHintedPlayers;
	
	ref map<string, EntityID> m_mPlayerGroups;
	
	protected bool m_bGameInitialized = false;
	protected bool m_bCameraSet = false;
	protected bool m_bGameStarted = false;
			
	bool IsInitialized()
	{
		return m_bGameInitialized;
	}
	
	void DoStartNewGame()
	{
		m_Persistence.StartNewGame();
		if(m_OccupyingFactionManager)
		{
			Print("Starting Occupying Faction");
			
			m_OccupyingFactionManager.NewGameStart();
		}	
	}
	
	OVT_PersistenceManagerComponent GetPersistence()
	{
		return m_Persistence;
	}
	
	void DoStartGame()
	{
		m_StartGameUIContext.CloseLayout();
		m_bGameStarted = true;
		
		if(RplSession.Mode() == RplMode.Dedicated)
		{
			Print("Spawning comms entity for dedicated server");					
			IEntity entity = GetGame().SpawnEntityPrefab(Resource.Load(m_PlayerCommsPrefab), GetGame().GetWorld());
			m_Server = OVT_PlayerCommsEntity.Cast(entity);
		}
		
		if(m_EconomyManager)
		{
			Print("Starting Economy");
			
			m_EconomyManager.PostGameStart();
		}
		
		if(m_TownManager)
		{
			Print("Starting Towns");
			
			m_TownManager.PostGameStart();
		}
				
		if(m_OccupyingFactionManager)
		{
			Print("Starting Occupying Faction");
			
			m_OccupyingFactionManager.PostGameStart();
		}	
		
		if(m_ResistanceFactionManager)
		{
			Print("Starting Resistance Faction");
			
			m_ResistanceFactionManager.PostGameStart();
		}	
		
		if(m_JobManager)
		{
			Print("Starting Jobs");
			
			m_JobManager.PostGameStart();
		}			
		
		Print("Overthrow Starting");
		m_bGameInitialized = true;
	}
	
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);
		
		if(DiagMenu.GetValue(200))
		{
			m_EconomyManager.DoAddPlayerMoney(SCR_PlayerController.GetLocalPlayerId(),1000);
			DiagMenu.SetValue(200,0);
		}
		
		if(DiagMenu.GetValue(201))
		{
			OVT_TownData town = OVT_Global.GetTowns().GetNearestTown(SCR_PlayerController.GetLocalControlledEntity().GetOrigin());
			if(town)
			{
				town.support = town.population;
			}
			DiagMenu.SetValue(201,0);
		}
		
		if(DiagMenu.GetValue(202))
		{
			OVT_TownData town = OVT_Global.GetTowns().GetNearestTown(SCR_PlayerController.GetLocalControlledEntity().GetOrigin());
			if(town)
			{
				OVT_Global.GetTowns().ChangeTownControl(town, m_Config.GetPlayerFactionIndex());
			}
			DiagMenu.SetValue(202,0);
		}
		
		if(DiagMenu.GetValue(203))
		{
			OVT_Global.GetOccupyingFaction().WinBattle();
			DiagMenu.SetValue(203,0);
		}
		
		if(DiagMenu.GetValue(204))
		{
			foreach(OVT_TownData town : m_TownManager.m_Towns)
			{
				int townID = OVT_Global.GetTowns().GetTownID(town);
				m_TownManager.TryAddSupportModifierByName(townID, "RecruitmentPosters");
				m_TownManager.TryAddSupportModifierByName(townID, "RecruitmentPosters");
				m_TownManager.TryAddSupportModifierByName(townID, "RecruitmentPosters");
				m_TownManager.TryAddSupportModifierByName(townID, "RecruitmentPosters");
				m_TownManager.TryAddSupportModifierByName(townID, "RecruitmentPosters");
			}
			DiagMenu.SetValue(204,0);
		}
		
		if(!(IsMaster() && RplSession.Mode() == RplMode.Dedicated) && !m_bCameraSet)
		{
			SetRandomCameraPosition();
		}
		
		if(m_bGameInitialized) return;
		m_StartGameUIContext.EOnFrame(owner, timeSlice);
	}
	
	void PreShutdownPersist()
	{		
		// Save all online player locations
		array<int> players = new array<int>;
		PlayerManager mgr = GetGame().GetPlayerManager();
		mgr.GetPlayers(players);
		foreach(int playerId : players){
			IEntity player = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			string persId = m_PlayerManager.GetPersistentIDFromPlayerID(playerId);
			OVT_PlayerData playerData = m_PlayerManager.GetPlayer(persId);
			if(!playerData) continue;
			
			playerData.location = player.GetOrigin();
		}
	}
	
	protected override void OnPlayerRoleChange(int playerId, EPlayerRole roleFlags)
	{
		super.OnPlayerRoleChange(playerId, roleFlags);
		
		if(SCR_Global.IsAdminRole(roleFlags))
		{
			string persId = m_PlayerManager.GetPersistentIDFromPlayerID(playerId);
			OVT_PlayerData player = m_PlayerManager.GetPlayer(persId);
			if(!player) return;
			if(!player.isOfficer)
			{
				m_ResistanceFactionManager.AddOfficer(playerId);
			}
		}
	}
	
	protected override void OnPlayerRegistered(int playerId)
	{
		super.OnPlayerRegistered(playerId);
		if(!Replication.IsServer()) return;
		
		PlayerController playerController = GetGame().GetPlayerManager().GetPlayerController(playerId);
		if(!playerController) return;
		
		//Print("Spawning player comms entity for player " + playerId);
		RplIdentity playerRplID = playerController.GetRplIdentity();		
		IEntity entity = GetGame().SpawnEntityPrefab(Resource.Load(m_PlayerCommsPrefab), GetGame().GetWorld());
		
		RplComponent rpl = RplComponent.Cast(entity.FindComponent(RplComponent));
		
		//Print("Assigning comms to player " + playerId);
		rpl.Give(playerRplID);
		
		m_TownManager.StreamTownModifiers(playerId);
		
		if(RplSession.Mode() == RplMode.Dedicated && !m_bGameStarted)
		{
			RemoteStartGame();
		}
	}
	
	protected override void OnPlayerKilled(int playerId, IEntity player, IEntity killer)
	{
		super.OnPlayerKilled(playerId, player, killer);
		
		string persId = m_PlayerManager.GetPersistentIDFromPlayerID(playerId);
		OVT_PlayerData playerData = m_PlayerManager.GetPlayer(persId);
		if(playerData)
		{
			playerData.location = OVT_Global.GetRealEstate().GetHome(persId);
		}
	}
	
	protected override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{		
		string persId = m_PlayerManager.GetPersistentIDFromPlayerID(playerId);
		IEntity controlledEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		
		if(controlledEntity)
		{
			OVT_PlayerData player = m_PlayerManager.GetPlayer(persId);
			if(player)
			{
				player.location = controlledEntity.GetOrigin();
				player.id = -1;
			}
			
			EPF_PersistenceComponent persistence = EPF_PersistenceComponent.Cast(controlledEntity.FindComponent(EPF_PersistenceComponent));
			if(persistence)
			{
				persistence.PauseTracking();
				persistence.Save();				
			}
		}
		
		int i = m_aInitializedPlayers.Find(persId);
		
		if(i > -1)
			m_aInitializedPlayers.Remove(i);
		
		super.OnPlayerDisconnected(playerId, cause, timeout);
	}
	
	override void OnPlayerAuditSuccess(int iPlayerID)
	{
		super.OnPlayerAuditSuccess(iPlayerID);
		
		string persistentId = EPF_Utils.GetPlayerUID(iPlayerID);
		m_PlayerManager.RegisterPlayer(iPlayerID, persistentId);
	}
	
	void PreparePlayer(int playerId, string persistentId)
	{
		
		OVT_PlayerData player = m_PlayerManager.GetPlayer(persistentId);
		if(!player) {
			m_PlayerManager.SetupPlayer(playerId, persistentId);
			player = m_PlayerManager.GetPlayer(persistentId);	
		}	
		
		if(!player.isOfficer && RplSession.Mode() == RplMode.None)
		{
			//In single player, make the player an officer
			m_ResistanceFactionManager.AddOfficer(playerId);
		}
		
#ifdef WORKBENCH
		if(!player.isOfficer && playerId == 1)
		{
			//In workbench, make the first player an officer
			m_ResistanceFactionManager.AddOfficer(playerId);
		}
#endif		
											
		if(player.initialized)
		{
			Print("Player exists, respawn");
			
			//Existing player	
			if(m_aInitializedPlayers.Contains(persistentId))
			{
				int cost = m_Config.m_Difficulty.respawnCost;
				m_EconomyManager.TakePlayerMoney(playerId, cost);
			}else{
				//This is a returning player, don't charge them hospital fees				
				m_aInitializedPlayers.Insert(persistentId);
			}
		}else{
			//New player
			Print("Adding start cash to player " + playerId);
			int cash = m_Config.m_Difficulty.startingCash;
			m_EconomyManager.AddPlayerMoney(playerId, cash);
			
			vector home = m_RealEstate.GetHome(persistentId);
			if(home[0] == 0)
			{
				Print("Adding home to player " + playerId);
				
				IEntity house = OVT_Global.GetTowns().GetRandomStartingHouse();
				m_RealEstate.SetOwner(playerId, house);
				m_RealEstate.SetHome(playerId, house);				
				player.location = house.GetOrigin();				
				
				Print("Spawning car for player " + playerId);
				m_VehicleManager.SpawnStartingCar(house, persistentId);
			}
			player.initialized = true;
			m_aInitializedPlayers.Insert(persistentId);
		}	
				
	}
	
	protected override void OnPlayerSpawned(int playerId, IEntity controlledEntity)
	{		
		OVT_PlayerWantedComponent wanted = OVT_PlayerWantedComponent.Cast(controlledEntity.FindComponent(OVT_PlayerWantedComponent));
		if(!wanted){
			Print("Player spawn prefab is missing OVT_PlayerWantedComponent!");
		}else{
			wanted.SetWantedLevel(0);
		}
	}
	
	protected void SetRandomCameraPosition()
	{
		CameraManager cameraMgr = GetGame().GetCameraManager();
		if(!cameraMgr) return;
		BaseWorld world = GetGame().GetWorld();
		
		int cameraIndex = s_AIRandomGenerator.RandInt(0, m_Config.m_aCameraPositions.Count()-1);
		OVT_CameraPosition pos = m_Config.m_aCameraPositions[cameraIndex];
		
		EntitySpawnParams params = EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;		
		params.Transform[3] = pos.position;
						
		IEntity cam = GetGame().SpawnEntityPrefabLocal(Resource.Load(m_StartCameraPrefab),null,params);
		if(cam)
		{			
			CameraBase camera = CameraBase.Cast(cam);
			camera.SetAngles(pos.angles);
			cameraMgr.SetCamera(camera);
			m_bCameraSet = true;
		}		
	}
	
	override void EOnInit(IEntity owner) //!EntityEvent.INIT
	{
		super.EOnInit(owner);
		
		DiagMenu.RegisterBool(200, "lctrl+lalt+g", "Give $1000", "Cheats");
		DiagMenu.SetValue(200, 0);
		
		DiagMenu.RegisterBool(201, "lctrl+lalt+s", "Give 100% support", "Cheats");
		DiagMenu.SetValue(201, 0);
		
		DiagMenu.RegisterBool(202, "lctrl+lalt+c", "Capture Town", "Cheats");
		DiagMenu.SetValue(202, 0);
		
		DiagMenu.RegisterBool(203, "lctrl+lalt+w", "Win Battle", "Cheats");
		DiagMenu.SetValue(203, 0);
		
		DiagMenu.RegisterBool(204, "lctrl+lalt+r", "Poster all towns", "Cheats");
		DiagMenu.SetValue(204, 0);
		
		if(SCR_Global.IsEditMode())
			return;		
								
		Print("Initializing Overthrow");
		
		m_Config = OVT_Global.GetConfig();				
		m_PlayerManager = OVT_Global.GetPlayers();		
		m_RealEstate = OVT_Global.GetRealEstate();
		
		m_TownManager = OVT_TownManagerComponent.Cast(FindComponent(OVT_TownManagerComponent));		
		if(m_TownManager)
		{
			Print("Initializing Towns");
			
			m_TownManager.Init(this);
		}
		
		m_EconomyManager = OVT_EconomyManagerComponent.Cast(FindComponent(OVT_EconomyManagerComponent));		
		if(m_EconomyManager)
		{
			Print("Initializing Economy");
			m_EconomyManager.Init(this);
		}
		
		m_OccupyingFactionManager = OVT_OccupyingFactionManager.Cast(FindComponent(OVT_OccupyingFactionManager));		
		if(m_OccupyingFactionManager)
		{
			Print("Initializing Occupying Faction");
			
			m_OccupyingFactionManager.Init(this);
		}
		
		m_ResistanceFactionManager = OVT_ResistanceFactionManager.Cast(FindComponent(OVT_ResistanceFactionManager));		
		if(m_ResistanceFactionManager)
		{
			Print("Initializing Resistance Faction");
			
			m_ResistanceFactionManager.Init(this);
		}
		
		m_VehicleManager = OVT_VehicleManagerComponent.Cast(FindComponent(OVT_VehicleManagerComponent));		
		if(m_VehicleManager)
		{
			Print("Initializing Vehicles");
			
			m_VehicleManager.Init(this);
		}	
		
		m_JobManager = OVT_JobManagerComponent.Cast(FindComponent(OVT_JobManagerComponent));		
		if(m_JobManager)
		{
			Print("Initializing Jobs");
			
			m_JobManager.Init(this);
		}	
		
		m_StartGameUIContext.Init(owner, null);
		m_StartGameUIContext.RegisterInputs();	
		
		if(!IsMaster()) {
			//show wait screen?
			return;
		}
		
		GetGame().GetTimeAndWeatherManager().SetDayDuration(86400 / m_Config.m_iTimeMultiplier);
		
		m_Persistence = OVT_PersistenceManagerComponent.Cast(FindComponent(OVT_PersistenceManagerComponent));		
		if(m_Persistence)
		{
			Print("Initializing Persistence");
			if(m_Persistence.HasSaveGame())
			{
				Print("Loading game");
				m_bCameraSet = true;
				RequestLoad();
				DoStartGame();
			}else{
				Print("No save game detected");
				if(RplSession.Mode() == RplMode.Dedicated)
				{
					Print("Dedicated server, starting new game");
					DoStartNewGame();
					DoStartGame();
				}else{
					m_StartGameUIContext.ShowLayout();
				}
			}
		}
	}
	
	void OnPlayerSpawnedLocal(string playerId)
	{
		if(!m_aHintedPlayers.Contains(playerId))
		{
			SCR_HintManagerComponent.GetInstance().ShowCustom("#OVT-IntroHint","#OVT-Overthrow",20);
			m_aHintedPlayers.Insert(playerId);
		}		
	}
	
	protected void RemoteStartGame()
	{
		//any players online yet?
		array<int> players = new array<int>;
		PlayerManager mgr = GetGame().GetPlayerManager();
		mgr.GetPlayers(players);
		int numplayers = mgr.GetPlayers(players);
		
		Print("Requested Remote Start Game. Players online: " + players.Count().ToString());
		
		if(players.Count() > 0)
		{
			//tell the first player to start the game
			Rpc(RpcDo_ShowStartGame, players[0]); 			
		}
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_ShowStartGame(int playerId)
	{
		int localId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(SCR_PlayerController.GetLocalControlledEntity());
		if(playerId != localId) return;
		
		m_StartGameUIContext.ShowLayout();
	}
	
	void RequestLoad()
	{
		m_Persistence.LoadGame();
	}
	
	void StartNewGame()
	{
		Print ("Overthrow: Start New Game Requested");
		DoStartNewGame();
		DoStartGame();
	}
	
	void StartGame()
	{
		Print("Overthrow: Requesting Start Game");
		Rpc(RpcAsk_StartGame);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_StartGame()
	{
		Print ("Overthrow: Start Game Requested");
		DoStartGame();
	}
	
	//------------------------------------------------------------------------------------------------
	void OVT_OverthrowGameMode(IEntitySource src, IEntity parent)
	{
		m_aInitializedPlayers = new set<string>;
		m_aHintedPlayers = new set<string>;
		m_mPlayerGroups = new map<string, EntityID>;
	}
	
	void ~OVT_OverthrowGameMode()
	{
		if(!m_PlayerManager) return;
		m_PlayerManager.m_OnPlayerRegistered.Remove(OnPlayerRegistered);
	}
}