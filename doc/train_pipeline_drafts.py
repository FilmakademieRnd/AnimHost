class ExternalTrainEngine:
    def train(self):
        if self.config.type == ExperimentType.Starke2022:
            self.train_starke2022()
        else:
            ValueError(f"Unsupported experiment type: {self.config.type}")

    def train_starke2022(self):
        run_pae_with_realtime_parsing(self.tracker)
        prepare_gnn_data()
        run_gnn_with_realtime_parsing(self.tracker)


class TrainingEngine:
    def train(self, eval_engine=None):
        for epoch in range(self.epochs):
            # Training loop
            train_metrics = self.train_epoch()
            
            # Optional intermediate evaluation
            if eval_engine and self.should_evaluate(epoch):
                val_metrics = eval_engine.evaluate()
                self.tracker.log_epoch({
                    **train_metrics, 
                    **val_metrics
                }, epoch)
            else:
                self.tracker.log_epoch(train_metrics, epoch)

class TrainFramework:
    def __init__(self):
        self.config_manager = ConfigManager()
        self.data_manager = DataManager()
        self.experiment_tracker = ExperimentTracker()
        self.model_manager = ModelManager()
        
    def run_training(self, data_config_path, model_config_path, experiment_config_path):
        config = self.config_manager.load_and_validate(
            model_config_path, data_config_path, experiment_config_path
        )
        if(config.model.type == ExperimentType.Starke2022):
            self.run_starke_2022_experiment(config)
        else:
            self.run_default(config)

    def run_starke_2022_experiment(self, config):       
        # 1. Initialize data and copy it to the expected dir
        data = self.data_manager.setup_datasets(config.data)
        # 1.1 Initialize data and copy it to the expected dir
        self.model_manager.initialize_starke2022_data(
            data, config.model.external_model_path
        )

        # 2. Initialize model by adjusting submodule code as per config input
        self.model_manager.initialize_starke2022_model(config.model)

        # 3. Setup experiment tracking
        tracker = self.experiment_tracker.start_run(config.experiment)
        
        # 4. Initialize engines
        train_engine = ExternalTrainEngine(config.model, tracker)

        train_engine.run()

    def run_default(self, config):      
        # 1. Initialize data pipeline
        data = self.data_manager.setup_datasets(config.data)
        
        # 2. Initalize the model
        model = self.model_manager.init(config.model)
        
        # 3. Setup experiment tracking
        tracker = self.experiment_tracker.start_run(config.experiment)
        
        # 4. Initialize engines
        train_engine = TrainingEngine(
            model, data, config.experiment, tracker
        )
        
        train_engine.run()


class EvalFramework:
    def __init__(self):
        self.config_manager = ConfigManager()
        self.data_manager = DataManager()
        self.experiment_tracker = ExperimentTracker()
        self.model_manager = ModelManager()
        
    def run_eval(self, data_config_path, model_config_path, eval_config_path):
        config = self.config_manager.load_and_validate(
            model_config_path, data_config_path, eval_config_path
        )
    
        # 1. Initialize data pipeline
        data = self.data_manager.setup_datasets(config.data)
        
        # 2. Load the model
        model = self.model_manager.load(config.model)
        
        # 3. Setup experiment tracking
        tracker = self.experiment_tracker.start_run(config.experiment)
        
        # 4. Initialize engines
        train_engine = EvalEngine(model, data, config.experiment, tracker)
        
        train_engine.run()